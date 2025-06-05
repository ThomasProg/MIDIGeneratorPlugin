// Copyright Prog'z. All Rights Reserved.

#include "GenThread.h"
#include "utilities.hpp"
#include "abstractPipeline.hpp"
#include "generationHistory.h"

FString FGenThread::RelativeToAbsoluteContentPath(const FString& BaseStr)
{
	if (BaseStr.Contains(":"))
	{
		return BaseStr;
	}
	else
	{
		FString FullPath = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*FPaths::ProjectContentDir());
		return FullPath + BaseStr;
	}
}

void FGenThread::SetPipeline(IAutoRegressivePipeline* NewPipeline)
{
	Pipeline = NewPipeline;
}

void FGenThread::PreStart(const FString& InTokenizerPath, const FString& InModelPath, const TArray<int32>& InTokens)
{
	this->TokenizerPath = RelativeToAbsoluteContentPath(InTokenizerPath);
	this->ModelPath = RelativeToAbsoluteContentPath(InModelPath);
	this->EncodedTokens = InTokens;
}

void FGenThread::Start(const FString& InTokenizerPath, const FString& InModelPath, const TArray<int32>& InTokens)
{
	PreStart(InTokenizerPath, InModelPath, InTokens);
	Start();
}

void FGenThread::Start()
{
	ensureMsgf(!HasStarted(), TEXT("FGenThread has already started!"));
	Thread = FRunnableThread::Create(this, TEXT("GenThread"), 0, TPri_TimeCritical);
}

bool FGenThread::HasStarted() const
{
	return Thread != nullptr;
}

void FGenThread::SetTokens(const TArray<int32>& InTokens)
{
	this->EncodedTokens = InTokens;
}

FGenThread::FGenThread()
{
	Semaphore = FGenericPlatformProcess::GetSynchEventFromPool(false);
}

FGenThread::~FGenThread()
{
	if (Semaphore)
	{
		Semaphore->Trigger();
		FGenericPlatformProcess::ReturnSynchEventToPool(Semaphore);
		Semaphore = nullptr;
	}

	if (Thread)
	{
		// Kill() is a blocking call, it waits for the thread to finish.
		// Hopefully that doesn't take too long
		Thread->Kill();
		delete Thread;
	}
}

bool FGenThread::Init()
{
	//if (GetTok() == nullptr)
	//{
	//	return false;
	//}
	//Generator.Init(TokenizerPath, ModelPath);

	//if (!Tokenizer.IsValid())
	//{
	//	Tokenizer = MakeShared<FTokenizerProxy>((MidiTokenizerHandle)nullptr);
	//}
	//Tokenizer->GetTokenizer()->LoadIfInvalid(TokenizerPath);

	if (Pipeline == nullptr)
	{
		env = createEnv(false);
		generator = createMusicGenerator();

		generator_setNbAttentionHeads(generator, 12);
		generator_setHiddenSize(generator, 768);
		generator_setNbLayers(generator, 12);
		generator_setNbMaxPositions(generator, 1024);
		generator_setVocabSize(generator, 50257);

		//generator_setNbAttentionHeads(generator, 4);
		//generator_setHiddenSize(generator, 256);
		//generator_setNbLayers(generator, 6);
		//generator_setNbMaxPositions(generator, 256);
		//generator_setVocabSize(generator, 30000);

		generator_loadOnnxModel(generator, env, TCHAR_TO_UTF8(*ModelPath));

		runInstance = generator_createRunInstance(GetGen());


		batch = createBatch();
		runInstance_addBatch(runInstance, batch);
		batch_set(batch, EncodedTokens.GetData(), EncodedTokens.Num(), 0);

		runInstance_setSearchStrategyData(runInstance, this);
		runInstance_setSearchStrategy(runInstance, [](const struct SearchArgs& args, void* searchStrategyData)
			{
				FGenThread* GenThread = (FGenThread*)searchStrategyData;
				GenThread->Mutex.Lock();
				FOnSearch OnSearch = GenThread->OnSearch;
				//void* SearchStrategyData = GenThread->SearchStrategyData;
				//TSearchStrategy SearchStrategy = GenThread->SearchStrategy;
				GenThread->Mutex.Unlock();

				OnSearch.Broadcast(args);

				//(*SearchStrategy)(args, SearchStrategyData);
			});
	}
	else
	{
		Batch2 = Pipeline->addBatch();
		Pipeline->batchSet(Batch2, EncodedTokens.GetData(), EncodedTokens.Num(), 0);
		Pipeline->setSearchStrategyData(this);
		Pipeline->setSearchStrategy([](const struct SearchArgs& args, void* searchStrategyData)
			{
				FGenThread* GenThread = (FGenThread*)searchStrategyData;
				GenThread->Mutex.Lock();
				FOnSearch OnSearch = GenThread->OnSearch;
				GenThread->Mutex.Unlock();
				OnSearch.Broadcast(args);
			});

		Pipeline->createHistory(*Tokenizer->GetTokenizer()->GetTokenizer());

		beatGenerator = createBeatGenerator();
	}


	OnInit.Broadcast();


	//runInstance_setSearchStrategyData(runInstance, SearchStrategyData);
	//runInstance_setSearchStrategy(runInstance, SearchStrategy);

	//runInstance_setSearchStrategyData(runInstance, this);
	//runInstance_setSearchStrategy(runInstance, [](const SearchArgs& args, void* searchStrategyData)
	//	{
	//		FGenThread& genThread = *(FGenThread*)searchStrategyData;

	//		//// pitch / velocity / duration

	//		//bool hasPitch = false;
	//		//bool hasVelocity = false;
	//		//bool hasDuration = false;

	//		//if (hasPitch && hasVelocity && hasDuration)
	//		//{
	//		//	hasPitch = false;
	//		//	hasVelocity = false;
	//		//	hasDuration = false;
	//		//}

	//		//auto filter = [](std::int32_t token, void* data) -> bool
	//		//{
	//		//	FGenThread& genThread = *(FGenThread*)data;

	//		//	std::int32_t* decodedTokens;
	//		//	std::int32_t nbDecodedTokens;
	//		//	// @TODO : thread safe
	//		//	tokenizer_decodeToken(genThread.Generator.tok, token, &decodedTokens, &nbDecodedTokens);

	//		//	for (std::int32_t i = 0; i < nbDecodedTokens; i++)
	//		//	{
	//		//		std::int32_t decodedToken = decodedTokens[i];
	//		//		if (isPitch(genThread.Generator.tok, decodedToken))
	//		//		{
	//		//			std::int32_t pitch = getPitch(genThread.Generator.tok, decodedToken);
	//		//			if (pitch > 80)
	//		//			{
	//		//				return false;
	//		//			}
	//		//		}
	//		//	}

	//		//	return true;

	//		//	//bool isIgnored = false;
	//		//	//isIgnored = isIgnored || (genThread.hasPitch && isPitch(genThread.Generator.tok, token));
	//		//	//isIgnored = isIgnored || (genThread.hasVelocity && isVelocity(genThread.Generator.tok, token));
	//		//	//isIgnored = isIgnored || (genThread.hasDuration && isDuration(genThread.Generator.tok, token));

	//		//	//if (isIgnored)
	//		//	//{
	//		//	//	return false;
	//		//	//}


	//		//};

	//		//MusicGenerator::getNextTokens_greedyFiltered(args, availableTokens->GetData(), availableTokens->Num());
	//		//generator_getNextTokens_greedyFiltered(args, filter, searchStrategyData);



	//		// Get the last token's logits for each sequence in the batch
	//		for (std::int32_t b = 0; b < args.nbBatches; ++b)
	//		{
	//			// Pointer to the logits for the last token
	//			const float* last_logits = args.logitsTensor + (b * args.nbSequences + (args.nbSequences - 1)) * args.vocabSize;

	//			// Find the index with the maximum logit
	//			float max_logit = last_logits[0];
	//			float max_logit2 = last_logits[0];
	//			std::int32_t max_index = 0;
	//			std::int32_t max_index2 = 0;
	//			for (std::int32_t token = 1; token < args.vocabSize; token++)
	//			{
	//				float added = 0.0;

	//				std::int32_t* decodedTokens = nullptr;
	//				std::int32_t nbDecodedTokens = 0;
	//				// @TODO : thread safe
	//				tokenizer_decodeToken(genThread.GetTok(), token, &decodedTokens, &nbDecodedTokens);
	//					
	//				for (std::int32_t i = 0; i < nbDecodedTokens; i++)
	//				{
	//					std::int32_t decodedToken = decodedTokens[i];
	//					if (isPitch(genThread.GetTok(), decodedToken))
	//					{
	//						std::int32_t pitch = getPitch(genThread.GetTok(), decodedToken);
	//						if (pitch > 80)
	//						{
	//							//added -= 0.5;
	//							break;
	//						}
	//					}
	//				}

	//				if (decodedTokens != nullptr)
	//					tokenizer_decodeToken_free(decodedTokens);

	//				if (last_logits[token] + added > max_logit)
	//				{
	//					max_logit2 = max_logit;
	//					max_logit = last_logits[token] + added;
	//					max_index2 = max_index;
	//					max_index = token;
	//				}
	//			}


	//			auto hasPitchX = [&](int32 token) -> bool
	//			{
	//				std::int32_t* decodedTokens = nullptr;
	//				std::int32_t nbDecodedTokens = 0;
	//				// @TODO : thread safe
	//				tokenizer_decodeToken(genThread.GetTok(), token, &decodedTokens, &nbDecodedTokens);

	//				for (std::int32_t i = 0; i < nbDecodedTokens; i++)
	//				{
	//					std::int32_t decodedToken = decodedTokens[i];

	//					if (isBarNone(genThread.GetTok(), decodedToken))
	//					{
	//						tokenizer_decodeIDs_free(decodedTokens);
	//						return false;
	//					}

	//					if (isPosition(genThread.GetTok(), decodedToken))
	//					{
	//						tokenizer_decodeIDs_free(decodedTokens);
	//						return false;
	//					}


	//					if (i == 0 && isPitch(genThread.GetTok(), decodedToken))
	//					{
	//						tokenizer_decodeIDs_free(decodedTokens);
	//						return true;
	//						//std::int32_t pitch = getPitch(genThread.Generator.tok, decodedToken);
	//						//if (pitch > 80)
	//						//{
	//						//	//added -= 0.5;
	//						//	break;
	//						//}
	//					}
	//				}

	//				if (decodedTokens != nullptr)
	//					tokenizer_decodeIDs_free(decodedTokens);
	//				
	//				return false;
	//			};



	//			if (hasPitchX(max_index) && hasPitchX(max_index2) && max_logit - max_logit2 < 0.5/*&& FMath::RandRange(0.0, 1.0) < 0.1*/)
	//				args.outNextTokens[b] = max_index2;
	//			else 
	//				args.outNextTokens[b] = max_index;
	//		}
	//	});


	//runInstance_setSearchStrategy(runInstance, [](const SearchArgs& args, void* searchStrategyData)
	//{
	//	// pitch / velocity / duration

	//	bool hasPitch = false;
	//	bool hasVelocity = false;
	//	bool hasDuration = false;

	//	TArray<int32> allTokens;
	//	TArray<int32> tokensWithoutPitch;
	//	TArray<int32> tokensWithoutVelocity;
	//	TArray<int32> tokensWithoutPitchAndVelocity;
	//	TArray<int32> tokensWithoutDuration;
	//	TArray<int32> tokensWithoutPitchAndDuration;
	//	TArray<int32> tokensWithoutVelocityAndDuration;

	//	if (hasPitch && hasVelocity && hasDuration)
	//	{
	//		hasPitch = false;
	//		hasVelocity = false;
	//		hasDuration = false;
	//	}

	//	int32 index = 0;
	//	index += (hasPitch ? 1 : 0);
	//	index += (hasVelocity ? 2 : 0);
	//	index += (hasDuration ? 4 : 0);

	//	TArray<TArray<int32>*, TFixedAllocator<7>> potentialAvailableTokens =
	//	{
	//		&allTokens,
	//		&tokensWithoutPitch,
	//		&tokensWithoutVelocity,
	//		&tokensWithoutPitchAndVelocity,
	//		&tokensWithoutDuration,
	//		&tokensWithoutPitchAndDuration,
	//		&tokensWithoutVelocityAndDuration,
	//	};

	//	TArray<int32>* availableTokens = potentialAvailableTokens[index];

	//	//MusicGenerator::getNextTokens_greedyFiltered(args, availableTokens->GetData(), availableTokens->Num());
	//	generator_getNextTokens_greedyPreFiltered(args, availableTokens->GetData(), availableTokens->Num());
	//});


	/* Should the thread start? */
	return true;
}

bool FGenThread::ShouldResumeGeneration() const
{
	GenerationHistory* History = Pipeline->getHistory(Batch2);
	const Note* outNotes;
	size_t outLength;
	generationHistory_getNotes(History, &outNotes, &outLength);
	if (outNotes == nullptr)
	{
		return true;
	}
	return outNotes[outLength - 1].tick < CurrentTick + NbMinTicksAhead;
}

bool FGenThread::ShouldSleep() const
{
	GenerationHistory* History = Pipeline->getHistory(Batch2);
	const Note* outNotes = nullptr;
	size_t outLength;
	generationHistory_getNotes(History, &outNotes, &outLength);
	if (outNotes == nullptr)
	{
		return false;
	}
	return outNotes[outLength - 1].tick >= CurrentTick + NbMaxTicksAhead;
}

uint32 FGenThread::Run()
{
	if (!forceReupdate)
	{
		TArray<int32> Context;
		//LineNbMaxToken = 512;
		int32 start = FMath::Max(0, EncodedTokens.Num() - LineNbMaxToken);
		for (int32 i = start; i < EncodedTokens.Num(); i++)
		{
			Context.Add(EncodedTokens[i]);
		}

		if (Pipeline != nullptr)
		{
			Pipeline->batchSet(Batch2, Context.GetData(), Context.Num(), start);
			Pipeline->setMaxInputLength(LineNbMaxToken);
		}
		else
		{
			batch_set(batch, Context.GetData(), Context.Num(), start);
			runInstance_setMaxInputLength(runInstance, LineNbMaxToken);
		}
	}

	NbTokensSinceLastRefresh = EncodedTokens.Num();

	while (!bShutdown)
	{
		SCOPE_CYCLE_COUNTER(STAT_GenThread);

		GenerationHistory* History = Pipeline->getHistory(Batch2);
		generationHistory_convert(History);

		if (!ShouldIgnoreNextToken.load(std::memory_order_acquire) && ShouldSleep())
		{
			const Note* outNotes = nullptr;
			size_t outLength;
			generationHistory_getNotes(History, &outNotes, &outLength);
			UE_LOG(LogTemp, Warning, TEXT("=== Pausing GenThread : Current: %d / Generated until: %d"), CurrentTick.load(), outNotes[outLength - 1].tick);
			Semaphore->Wait();
			generationHistory_getNotes(History, &outNotes, &outLength);
			UE_LOG(LogTemp, Warning, TEXT("=== Resuming GenThread : Current: %d / Generated until: %d"), CurrentTick.load(), outNotes[outLength - 1].tick);
		}

		if (forceReupdate)
		{
			if (Pipeline != nullptr)
			{
				Pipeline->reset();
			}
			else
			{
				runInstance_reset(runInstance);
			}

			{
				TArray<int32> Context;
				int32 start = FMath::Max(0, EncodedTokens.Num() - LineNbMaxToken);
				for (int32 i = start; i < EncodedTokens.Num(); i++)
				{
					Context.Add(EncodedTokens[i]);
				}

				if (Pipeline != nullptr)
				{
					Pipeline->batchSet(Batch2, Context.GetData(), Context.Num(), start);
				}
				else
				{
					batch_set(batch, Context.GetData(), Context.Num(), start);
				}
			}
		}

		Mutex.Lock();
		if (!OnSearch.IsBound())
		{
			Mutex.Unlock();
			continue;
		}
		Mutex.Unlock();

		if (ShouldRemoveTokens.load(std::memory_order_acquire))
		{
			ShouldRemoveTokens = false;
			ShouldIgnoreNextToken = false;
			RemoveCacheAfterTickInternal();
		}

		if (Pipeline != nullptr)
		{
			CppResult Result;
			Pipeline->preGenerate(Result);
			if (!Result.IsSuccess())
			{
				UE_LOG(LogTemp, Error, TEXT("An error occurred in function %s!\n%hs"), *FString(__FUNCTION__), Result.GetError());
				return -1;
			}

			Pipeline->generate(Result);
			if (!Result.IsSuccess())
			{
				UE_LOG(LogTemp, Error, TEXT("An error occurred in function %s!\n%hs"), *FString(__FUNCTION__), Result.GetError());
				return -1;
			}

			if (ShouldIgnoreNextToken.load(std::memory_order_acquire))
			{
				continue;
			}

			Pipeline->postGenerate(Result);
			if (!Result.IsSuccess())
			{
				UE_LOG(LogTemp, Error, TEXT("An error occurred in function %s!\n%hs"), *FString(__FUNCTION__), Result.GetError());
				return -1;
			}
		}
		else
		{
			{
				CppResult Res = generator_preGenerate(GetGen(), runInstance);
				if (!Res.IsSuccess())
				{
					UE_LOG(LogTemp, Error, TEXT("An error occurred in function %s!\n%hs"), *FString(__FUNCTION__), Res.GetError());
					return -1;
				}
			}

			{
				CppResult Res = generator_generate(GetGen(), runInstance);
				if (!Res.IsSuccess())
				{
					UE_LOG(LogTemp, Error, TEXT("An error occurred in function %s!\n%hs"), *FString(__FUNCTION__), Res.GetError());
					return -1;
				}
			}

			{
				CppResult Res = generator_postGenerate(GetGen(), runInstance);
				if (!Res.IsSuccess())
				{
					UE_LOG(LogTemp, Error, TEXT("An error occurred in function %s!\n%hs"), *FString(__FUNCTION__), Res.GetError());
					return -1;
				}
			}
		}

		if (ShouldIgnoreNextToken.load(std::memory_order_acquire))
		{
			continue;
		}

		int32 newToken;
		if (Pipeline != nullptr)
		{
			newToken = Pipeline->batchGetLastGeneratedToken(Batch2);
		}
		else
		{
			newToken = batch_getLastGeneratedToken(batch);
		}
		EncodedTokens.Add(newToken);
		NbTokensSinceLastRefresh++;

		//TryInsertTokenGroup();

		if (!bShutdown)
		{
			if (ShouldIgnoreNextToken.load(std::memory_order_acquire))
			{
				continue;
			}

			Mutex.Lock();
			OnGenerated.Broadcast(newToken);
			Mutex.Unlock();
		}
	}

	return 0;
}

void FGenThread::Exit() 
{
	if (beatGenerator)
	{
		destroyBeatGenerator(beatGenerator);
	}

	if (Pipeline == nullptr)
	{
		runInstance_removeBatch(runInstance, batch);
		destroyBatch(batch);
		destroyRunInstance(runInstance);
	}

	if (generator != nullptr)
	{
		destroyMusicGenerator(generator);
	}
	
	if (env != nullptr)
	{
		destroyEnv(env);
	}

}

void FGenThread::Stop()
{
	if (Semaphore)
	{
		Semaphore->Trigger();
	}
	bShutdown = true;
}

void FGenThread::RemoveCacheAfterTickInternal()
{
	int32 CacheTickToRemoveValue = CacheTickToRemove;
	Pipeline->batchRewind(Batch2, CacheTickToRemoveValue);
	//beatGenerator_rewind(beatGenerator, CacheTickToRemoveValue);
	OnCacheRemoved.Broadcast(CacheTickToRemoveValue);
}

void FGenThread::RemoveCacheAfterTick(int32 GenLibTick, float Ms)
{
	GenerationHistory* History = Pipeline->getHistory(Batch2);
	const Note* outNotes = nullptr;
	size_t outLength;
	generationHistory_getNotes(History, &outNotes, &outLength);
	if (outNotes == nullptr)
	{
		return;
	}

	if (CacheTickToRemove > outNotes[outLength - 1].tick)
	{
		return;
	}
	
	ShouldIgnoreNextToken.store(true, std::memory_order_release);

	CacheTickToRemove = GenLibTick;
	CacheMsToRemove = Ms;

	ShouldRemoveTokens.store(true, std::memory_order_release);
	Semaphore->Trigger();
}

//void FGenThread::SetSearchStrategy(void* InSearchStrategyData, TSearchStrategy InSearchStrategy)
//{
//	Mutex.Lock();
//	SearchStrategyData = InSearchStrategyData;
//	SearchStrategy = InSearchStrategy;
//	Mutex.Unlock();
//}

void FGenThread::SetSearchStrategy(TFunction<void(const struct SearchArgs& args)> InOnSearch)
{
	Mutex.Lock();
	OnSearch.AddLambda([OnSearchParam = MoveTemp(InOnSearch)](const struct SearchArgs& args) { OnSearchParam(args); });
	Mutex.Unlock();
}

void FGenThread::SetOnGenerated(TFunction<void(int32 NewToken)> InOnGenerated)
{
	Mutex.Lock();
	OnGenerated.AddLambda([OnGeneratedParam = MoveTemp(InOnGenerated)](int32 newToken) { OnGeneratedParam(newToken); });
	Mutex.Unlock();
}

void FGenThread::SetOnInit(TFunction<void()> InOnInit)
{
	Mutex.Lock();
	OnInit.AddLambda([InOnInitParam = MoveTemp(InOnInit)]() { InOnInitParam(); });
	Mutex.Unlock();
}

//TSharedPtr<Audio::IProxyData> FGenThread::CreateProxyData(const Audio::FProxyDataInitParams& InitParams)
//{
//	return MakeShared<FMIDIGeneratorProxy, ESPMode::ThreadSafe>(this);
//}
