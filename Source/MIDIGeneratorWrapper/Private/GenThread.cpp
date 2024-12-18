// Fill out your copyright notice in the Description page of Project Settings.


#include "GenThread.h"

void FGenThread::PreStart(const FString& InTokenizerPath, const FString& InModelPath, const TArray<int32>& InTokens)
{
	this->TokenizerPath = InTokenizerPath;
	this->ModelPath = InModelPath;
	this->EncodedLine = InTokens;
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

FGenThread::~FGenThread()
{
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
	Generator.Init(TokenizerPath, ModelPath);
	runInstance = generator_createRunInstance(Generator.generator);
	//runInstance = createRunInstance();
	batch = createBatch();
	runInstance_addBatch(runInstance, batch);
	batch_set(batch, EncodedLine.GetData(), EncodedLine.Num(), 0);

	runInstance_setSearchStrategyData(runInstance, this);

	runInstance_setSearchStrategy(runInstance, [](const SearchArgs& args, void* searchStrategyData)
		{
			FGenThread& genThread = *(FGenThread*)searchStrategyData;

			// pitch / velocity / duration

			bool hasPitch = false;
			bool hasVelocity = false;
			bool hasDuration = false;

			if (hasPitch && hasVelocity && hasDuration)
			{
				hasPitch = false;
				hasVelocity = false;
				hasDuration = false;
			}

			auto filter = [](std::int32_t token, void* data) -> bool
			{
				FGenThread& genThread = *(FGenThread*)data;

				std::int32_t* decodedTokens;
				std::int32_t nbDecodedTokens;
				// @TODO : thread safe
				tokenizer_decodeToken(genThread.Generator.tok, token, &decodedTokens, &nbDecodedTokens);

				for (std::int32_t i = 0; i < nbDecodedTokens; i++)
				{
					std::int32_t decodedToken = decodedTokens[i];
					if (isPitch(genThread.Generator.tok, decodedToken))
					{
						std::int32_t pitch = getPitch(genThread.Generator.tok, decodedToken);
						if (pitch > 80)
						{
							return false;
						}
					}
				}

				return true;

				//bool isIgnored = false;
				//isIgnored = isIgnored || (genThread.hasPitch && isPitch(genThread.Generator.tok, token));
				//isIgnored = isIgnored || (genThread.hasVelocity && isVelocity(genThread.Generator.tok, token));
				//isIgnored = isIgnored || (genThread.hasDuration && isDuration(genThread.Generator.tok, token));

				//if (isIgnored)
				//{
				//	return false;
				//}


			};

			//MusicGenerator::getNextTokens_greedyFiltered(args, availableTokens->GetData(), availableTokens->Num());
			//generator_getNextTokens_greedyFiltered(args, filter, searchStrategyData);



			// Get the last token's logits for each sequence in the batch
			for (std::int32_t b = 0; b < args.nbBatches; ++b)
			{
				// Pointer to the logits for the last token
				const float* last_logits = args.logitsTensor + (b * args.nbSequences + (args.nbSequences - 1)) * args.vocabSize;

				// Find the index with the maximum logit
				float max_logit = last_logits[0];
				float max_logit2 = last_logits[0];
				std::int32_t max_index = 0;
				std::int32_t max_index2 = 0;
				for (std::int32_t token = 1; token < args.vocabSize; token++)
				{
					float added = 0.0;

					std::int32_t* decodedTokens = nullptr;
					std::int32_t nbDecodedTokens = 0;
					// @TODO : thread safe
					tokenizer_decodeToken(genThread.Generator.tok, token, &decodedTokens, &nbDecodedTokens);

					for (std::int32_t i = 0; i < nbDecodedTokens; i++)
					{
						std::int32_t decodedToken = decodedTokens[i];
						if (isPitch(genThread.Generator.tok, decodedToken))
						{
							std::int32_t pitch = getPitch(genThread.Generator.tok, decodedToken);
							if (pitch > 80)
							{
								//added -= 0.5;
								break;
							}
						}
					}

					if (decodedTokens != nullptr)
						tokenizer_decodeIDs_free(decodedTokens);

					if (last_logits[token] + added > max_logit)
					{
						max_logit2 = max_logit;
						max_logit = last_logits[token] + added;
						max_index2 = max_index;
						max_index = token;
					}
				}


				auto hasPitchX = [&](int32 token) -> bool
				{
					std::int32_t* decodedTokens = nullptr;
					std::int32_t nbDecodedTokens = 0;
					// @TODO : thread safe
					tokenizer_decodeToken(genThread.Generator.tok, token, &decodedTokens, &nbDecodedTokens);

					for (std::int32_t i = 0; i < nbDecodedTokens; i++)
					{
						std::int32_t decodedToken = decodedTokens[i];

						if (isBarNone(genThread.Generator.tok, decodedToken))
						{
							tokenizer_decodeIDs_free(decodedTokens);
							return false;
						}

						if (isPosition(genThread.Generator.tok, decodedToken))
						{
							tokenizer_decodeIDs_free(decodedTokens);
							return false;
						}


						if (i == 0 && isPitch(genThread.Generator.tok, decodedToken))
						{
							tokenizer_decodeIDs_free(decodedTokens);
							return true;
							//std::int32_t pitch = getPitch(genThread.Generator.tok, decodedToken);
							//if (pitch > 80)
							//{
							//	//added -= 0.5;
							//	break;
							//}
						}
					}

					if (decodedTokens != nullptr)
						tokenizer_decodeIDs_free(decodedTokens);
					
					return false;
				};



				if (hasPitchX(max_index) && hasPitchX(max_index2) && max_logit - max_logit2 < 0.5/*&& FMath::RandRange(0.0, 1.0) < 0.1*/)
					args.outNextTokens[b] = max_index2;
				else 
					args.outNextTokens[b] = max_index;
			}
		});


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

void FGenThread::AddTokenGroupToInsert(const TArray<int32>& TokenGroup)
{
	TokenGroupToInsert = TokenGroup;
}

void FGenThread::TryInsertTokenGroup()
{
	//TArray<int32> DecodedLine = Decode(EncodedLine);

	//int32 indexAfterTick = FindIndexAfterTick(DecodedLine, TokenGroupInsertTick);
	//if (indexAfterTick < DecodedLine.Num())
	//{
	//	DecodedLine.SetNum(indexAfterTick, EAllowShrinking::No);

	//	for (int32 token : TokenGroupToInsert)
	//	{
	//		DecodedLine.Push(token);
	//	}
	//}

	//EncodedLine = Encode(DecodedLine);
}

uint32 FGenThread::Run() 
{
	if (!forceReupdate)
	{
		TArray<int32> Context;
		//LineNbMaxToken = 512;
		int32 start = FMath::Max(0, EncodedLine.Num() - LineNbMaxToken);
		for (int32 i = start; i < EncodedLine.Num(); i++)
		{
			Context.Add(EncodedLine[i]);
		}

		batch_set(batch, Context.GetData(), Context.Num(), start);

		runInstance_setMaxInputLength(runInstance, LineNbMaxToken);
	}

	while (!bShutdown) 
	{
		SCOPE_CYCLE_COUNTER(STAT_GenThread);

		if (forceReupdate)
		{
			runInstance_reset(runInstance);

			{
				TArray<int32> Context;
				int32 start = FMath::Max(0, EncodedLine.Num() - LineNbMaxToken);
				for (int32 i = start; i < EncodedLine.Num(); i++)
				{
					Context.Add(EncodedLine[i]);
				}

				batch_set(batch, Context.GetData(), Context.Num(), start);
			}
		}

		//generator_generateNextToken(Generator.generator, runInstance);

		generator_preGenerate(Generator.generator, runInstance);
		generator_generate(Generator.generator, runInstance);

		//const float* tensor = runInstance_getPastTensor(runInstance, 0);
		//const float* presTensor = runInstance_getPresentTensor(runInstance, 0);

		//int64 nbPastValues = FMath::Min(LineNbMaxToken, EncodedLine.Num());

		//auto getValue = [](int64 indices[], int64 dims[])
		//	{
		//		return indices[4] + indices[3] * dims[4]
		//			+ indices[2] * dims[3] * dims[4] + indices[1] * dims[2] * dims[3] * dims[4]
		//			+ indices[0] * dims[1] * dims[2] * dims[3] * dims[4];
		//	};

		//if (nbPastValues > 3)
		//{
		//	if (tensor != nullptr)
		//	{
		//		int64 dims[5] = { 2, 1, 4, nbPastValues, 64 };

		//		int x = 0;
		//		int batch2 = 0;
		//		int head = 0;
		//		int v = 3;
		//		int embd = 0;

		//		int64 in1[5] = { 0,0,0,3,0 };
		//		int id = getValue(in1, dims);
		//		int64 in2[5] = { 0,0,0,2,0 };
		//		int id2 = getValue(in2, dims);
		//		int64 in3[5] = { 0,0,0,1,0 };
		//		int id3 = getValue(in3, dims);

		//		float f = tensor[id];
		//		float f2 = tensor[id2];
		//		float f3 = tensor[id3];


		//		float vxx = f;
		//	}

		//	if (presTensor != nullptr)
		//	{
		//		int64 dims[5] = { 2, 1, 4, nbPastValues, 64 };

		//		int64 in1[5] = { 0,0,0,3,0 };
		//		int id = getValue(in1, dims);
		//		int64 in2[5] = { 0,0,0,2,0 };
		//		int id2 = getValue(in2, dims);
		//		int64 in3[5] = { 0,0,0,1,0 };
		//		int id3 = getValue(in3, dims);

		//		float f = presTensor[id];
		//		float f2 = presTensor[id2];
		//		float f3 = presTensor[id3];


		//		float vxx = f;
		//	}
		//}

		generator_postGenerate(Generator.generator, runInstance);

		//int32* tokens;
		//int32 tokensSize;
		//batch_getEncodedTokens(batch, &tokens, &tokensSize);

		//int32 newToken = generateNextToken(Context);
		//int32 newToken = tokens[tokensSize - 1];
		int32 newToken = batch_getLastGeneratedToken(batch);
		EncodedLine.Add(newToken);


		TryInsertTokenGroup();


		//Context.Add(newToken);

		//if (Context.Num() > LineNbMaxToken)
		//{
		//	Context.RemoveAt(0, EAllowShrinking::No);
		//}

		//// while generated ahead enough
		//while (EncodedLine.Num() - NextTokenIndexToPlay > NbMaxTokensAhead)
		//{
		//	FPlatformProcess::Sleep(0.1f);
		//}

		if (!bShutdown)
			OnGenerated.ExecuteIfBound(newToken);
	}

	return 0;
}

void FGenThread::Exit() 
{
	runInstance_removeBatch(runInstance, batch);
	destroyBatch(batch);
	destroyRunInstance(runInstance);

	Generator.Deinit();
}

void FGenThread::Stop() 
{
	bShutdown = true;
}

//TSharedPtr<Audio::IProxyData> FGenThread::CreateProxyData(const Audio::FProxyDataInitParams& InitParams)
//{
//	return MakeShared<FMIDIGeneratorProxy, ESPMode::ThreadSafe>(this);
//}
