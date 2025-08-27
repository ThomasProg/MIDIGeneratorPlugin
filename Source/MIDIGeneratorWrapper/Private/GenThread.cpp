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
			GenThread->Mutex.Unlock();

			OnSearch.Broadcast(args);
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

		BeatGeneratorMutex.Lock();
		beatGenerator = createBeatGenerator();
		BeatGeneratorMutex.Unlock();
	}


	OnInit.Broadcast();

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
		generationHistory_convertToNotes(History);

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
		BeatGeneratorMutex.Lock();
		destroyBeatGenerator(beatGenerator);
		BeatGeneratorMutex.Unlock();
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
	BeatGeneratorMutex.Lock();
	beatGenerator_rewind(beatGenerator, CacheTickToRemoveValue);
	BeatGeneratorMutex.Unlock();
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

void FGenThread::AddOnInit(TFunction<void()> InOnInit)
{
	Mutex.Lock();
	OnInit.AddLambda([InOnInitParam = MoveTemp(InOnInit)]() { InOnInitParam(); });
	Mutex.Unlock();
}
