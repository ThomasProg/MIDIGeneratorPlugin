// Fill out your copyright notice in the Description page of Project Settings.


#include "MIDIGeneratorEnv.h"
#include "GenThread.h"
#include "HarmonixMetasound/DataTypes/MidiClock.h"
#include "HarmonixMetasound/DataTypes/MusicTimeInterval.h"

FMIDIGeneratorProxy::FMIDIGeneratorProxy(UMIDIGeneratorEnv* InGeneratorEnv)
{
	if (InGeneratorEnv && InGeneratorEnv->Generator)
	{
		MidiGenerator = InGeneratorEnv->Generator->MidiGenerator;
	}
}

FMIDIGeneratorProxy::FMIDIGeneratorProxy(const TSharedPtr<FMIDIGeneratorEnv>& InGeneratorEnv)
{
	if (InGeneratorEnv)
	{
		MidiGenerator = InGeneratorEnv;
	}
}

FMIDIGeneratorEnv::~FMIDIGeneratorEnv()
{
	//if (RangeGroup)
	//{
	//	destroyRangeGroup(RangeGroup);
	//}
}

void FMIDIGeneratorEnv::PreStart(const FString& TokenizerPath, const FString& ModelPath, const TArray<int32>& InTokens)
{
	GenThread->PreStart(TokenizerPath, ModelPath, InTokens);
}

void FMIDIGeneratorEnv::StartGeneration()
{
	//GenThread->Start();

	if (!GenThread->HasStarted())
	{
		int32 CurrentTempo = 120;
		int32 CurrentTimeSigNum = 4;
		int32 CurrentTimeSigDenom = 4;
		MidiFileData = HarmonixMetasound::FMidiClock::MakeClockConductorMidiData(CurrentTempo, CurrentTimeSigNum, CurrentTimeSigDenom);

		MidiFileData->MidiFileName = "Generated";
		MidiFileData->TicksPerQuarterNote = 16;
		FMidiTrack track;
		track.AddEvent(FMidiEvent(0, FMidiMsg::CreateAllNotesKill()));
		MidiFileData->Tracks.Add(MoveTemp(track));

		MidiDataProxy = MakeShared<FMidiFileProxy, ESPMode::ThreadSafe>(MidiFileData);

		GenThread->SetOnGenerated(FOnGenerated::CreateLambda([this](int32 NewToken)
		{
			EncodedTokensSection.Lock();
			bShouldUpdateTokens = true;
			NewEncodedTokens.Add(NewToken);
			EncodedTokensSection.Unlock();
		}));

		GenThread->Start();
	}
}

void FMIDIGeneratorEnv::StopGeneration()
{
	GenThread->Stop();
	GenThread->SetOnGenerated(nullptr);
}

void FMIDIGeneratorEnv::SetFilter()
{
	//if (CurrentRangeGroup == nullptr)
	//{
	//	CurrentRangeGroup = createRangeGroup();
	//}

	MidiTokenizerHandle tok = GenThread->GetTok();

	if (tok == nullptr)
	{
		return;
	}

	RangeGroupHandle BaseRangeGroup = createRangeGroup();

	tokenizer_addTokensStartingByPosition(tok, BaseRangeGroup);
	tokenizer_addTokensStartingByBarNone(tok, BaseRangeGroup);

	RangeGroupHandle PitchRangeGroup = cloneRangeGroup(BaseRangeGroup);
	tokenizer_addTokensStartingByPitch(tok, PitchRangeGroup);

	RangeGroupHandle VelocityRangeGroup = cloneRangeGroup(BaseRangeGroup);
	tokenizer_addTokensStartingByVelocity(tok, VelocityRangeGroup);

	RangeGroupHandle DurationRangeGroup = cloneRangeGroup(BaseRangeGroup);
	tokenizer_addTokensStartingByDuration(tok, DurationRangeGroup);

	RangeGroupHandle AllRangeGroup = cloneRangeGroup(BaseRangeGroup);
	tokenizer_addTokensStartingByPitch(tok, AllRangeGroup);
	tokenizer_addTokensStartingByVelocity(tok, AllRangeGroup);
	tokenizer_addTokensStartingByDuration(tok, AllRangeGroup);

	CurrentRangeGroup = AllRangeGroup;



	Range const* ranges;
	size_t nbRanges;
	rangeGroupGetRanges(CurrentRangeGroup, &ranges, &nbRanges);




	GenThread->SetSearchStrategy(FOnSearch::CreateLambda([this](const SearchArgs& args)
		{
			//MidiTokenizerHandle tok = GenThread->GetTok();

			Range const * ranges;
			size_t nbRanges;
			rangeGroupGetRanges(CurrentRangeGroup, &ranges, &nbRanges);

			check(args.nbBatches == 1);
			// Get the last token's logits for each sequence in the batch
			for (std::int32_t b = 0; b < args.nbBatches; ++b)
			{
				// Start of the batch
				const float* batchLogits = args.logitsTensor + (b * args.nbSequences + (args.nbSequences - 1)) * args.vocabSize;

				// Find the index with the maximum logit
				float maxLogit = -FLT_MAX;
				std::int32_t max_index = 0;
				for (std::int32_t rangeIndex = 0; rangeIndex < nbRanges; rangeIndex++)
				{
					for (std::int32_t token = ranges[rangeIndex].min; token <= ranges[rangeIndex].max; token++)
					{
						float currentLogit = batchLogits[token];

						//float added = 0.0;

						//std::int32_t* decodedTokens = nullptr;
						//std::int32_t nbDecodedTokens = 0;
						//// @TODO : thread safe
						//tokenizer_decodeToken(tok, token, &decodedTokens, &nbDecodedTokens);

						//for (std::int32_t i = 0; i < nbDecodedTokens; i++)
						//{
						//	std::int32_t decodedToken = decodedTokens[i];
						//	if (isPitch(tok, decodedToken))
						//	{
						//		std::int32_t pitch = getPitch(tok, decodedToken);
						//		if (pitch > 80)
						//		{
						//			//added -= 0.5;
						//			break;
						//		}
						//	}
						//}

						//if (decodedTokens != nullptr)
						//	tokenizer_decodeIDs_free(decodedTokens);

						if (batchLogits[token] > maxLogit)
						{
							maxLogit = batchLogits[token];
							max_index = token;
						}
					}
				}
				args.outNextTokens[b] = max_index;
			}
		}));















	//GenThread->SearchStrategyData = this;
	//GenThread->SearchStrategy = [](const SearchArgs& args, void* searchStrategyData)
	//{
	//	FGenThread& genThread = *(FGenThread*)searchStrategyData;
	//	MidiTokenizerHandle tok = genThread.Generator.tok;

	//	//generator_getNextTokens_greedyFiltered(args, filter, searchStrategyData);

	//	// Get the last token's logits for each sequence in the batch
	//	for (std::int32_t b = 0; b < args.nbBatches; ++b)
	//	{
	//		// Pointer to the logits for the last token
	//		const float* last_logits = args.logitsTensor + (b * args.nbSequences + (args.nbSequences - 1)) * args.vocabSize;

	//		// Find the index with the maximum logit
	//		float max_logit = last_logits[0];
	//		float max_logit2 = last_logits[0];
	//		std::int32_t max_index = 0;
	//		std::int32_t max_index2 = 0;
	//		for (std::int32_t token = 1; token < args.vocabSize; token++)
	//		{
	//			float added = 0.0;

	//			std::int32_t* decodedTokens = nullptr;
	//			std::int32_t nbDecodedTokens = 0;
	//			// @TODO : thread safe
	//			tokenizer_decodeToken(genThread.Generator.tok, token, &decodedTokens, &nbDecodedTokens);

	//			for (std::int32_t i = 0; i < nbDecodedTokens; i++)
	//			{
	//				std::int32_t decodedToken = decodedTokens[i];
	//				if (isPitch(genThread.Generator.tok, decodedToken))
	//				{
	//					std::int32_t pitch = getPitch(genThread.Generator.tok, decodedToken);
	//					if (pitch > 80)
	//					{
	//						//added -= 0.5;
	//						break;
	//					}
	//				}
	//			}

	//			if (decodedTokens != nullptr)
	//				tokenizer_decodeIDs_free(decodedTokens);

	//			if (last_logits[token] + added > max_logit)
	//			{
	//				max_logit2 = max_logit;
	//				max_logit = last_logits[token] + added;
	//				max_index2 = max_index;
	//				max_index = token;
	//			}
	//		}


	//		auto hasPitchX = [&](int32 token) -> bool
	//			{
	//				std::int32_t* decodedTokens = nullptr;
	//				std::int32_t nbDecodedTokens = 0;
	//				// @TODO : thread safe
	//				tokenizer_decodeToken(genThread.Generator.tok, token, &decodedTokens, &nbDecodedTokens);

	//				for (std::int32_t i = 0; i < nbDecodedTokens; i++)
	//				{
	//					std::int32_t decodedToken = decodedTokens[i];

	//					if (isBarNone(genThread.Generator.tok, decodedToken))
	//					{
	//						tokenizer_decodeIDs_free(decodedTokens);
	//						return false;
	//					}

	//					if (isPosition(genThread.Generator.tok, decodedToken))
	//					{
	//						tokenizer_decodeIDs_free(decodedTokens);
	//						return false;
	//					}


	//					if (i == 0 && isPitch(genThread.Generator.tok, decodedToken))
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

	//				return false;
	//			};



	//		if (hasPitchX(max_index) && hasPitchX(max_index2) && max_logit - max_logit2 < 0.5/*&& FMath::RandRange(0.0, 1.0) < 0.1*/)
	//			args.outNextTokens[b] = max_index2;
	//		else
	//			args.outNextTokens[b] = max_index;
	//	}
	//};
}

UMIDIGeneratorEnv::UMIDIGeneratorEnv()
{
	Generator = MakeShared<FMIDIGeneratorProxy, ESPMode::ThreadSafe>(nullptr);
	Generator->MidiGenerator = MakeShared<FMIDIGeneratorEnv>();
}

void UMIDIGeneratorEnv::StartGeneration()
{
	Generator->MidiGenerator->StartGeneration();
}
void UMIDIGeneratorEnv::StopGeneration()
{
	Generator->MidiGenerator->StopGeneration();
}

void UMIDIGeneratorEnv::PreStart(const FString& TokenizerPath, const FString& ModelPath, const TArray<int32>& InTokens)
{
	Generator->MidiGenerator->PreStart(TokenizerPath, ModelPath, InTokens);
}

void UMIDIGeneratorEnv::SetFilter()
{
	Generator->MidiGenerator->SetFilter();
}

TSharedPtr<Audio::IProxyData> UMIDIGeneratorEnv::CreateProxyData(const Audio::FProxyDataInitParams& InitParams)
{
	return MakeShared<FMIDIGeneratorProxy, ESPMode::ThreadSafe>(this);
}