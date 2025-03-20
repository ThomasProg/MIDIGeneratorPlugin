// Fill out your copyright notice in the Description page of Project Settings.


#include "MIDIGeneratorEnv.h"
#include "GenThread.h"
#include "HarmonixMetasound/DataTypes/MidiClock.h"
#include "HarmonixMetasound/DataTypes/MusicTimeInterval.h"

#include "modelBuilderManager.hpp"
#include "abstractPipeline.hpp"
#include "logitProcessing.h"
#include "logitProcessing.hpp"

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

void FMIDIGeneratorEnv::PreloadPipeline(const FString& ModelPath)
{
	const char* Path = TCHAR_TO_UTF8(*GenThread->RelativeToAbsoluteContentPath(ModelPath));

	EnvHandle env = createEnv(false);

	ModelLoadingParamsWrapper params;
	CResult r = createModelLoadingParamsWrapperFromFolder(Path, &params);
	if (!ResultIsSuccess(&r))
	{
		verify(false); // couldn't load file
		return;
	}

	//CStr ModelType = modelLoadingParams_getModelType(&params);
	CppStr ModelType = params.getModelType();
	OnnxModelBuilder* Builder = getModelBuilderManager().findBuilder<OnnxModelBuilder>(ModelType.Str());
	Builder->env = env;

	AModel* Model = Builder->loadModelFromWrapper(params);
	IPipeline* Pipeline = Model->createPipeline();
	IAutoRegressivePipeline* ARPipeline = (IAutoRegressivePipeline*)Pipeline; // @TODO : dynamic cast

	GenThread->SetPipeline(ARPipeline);
}

void FMIDIGeneratorEnv::SetTokens(const TArray<int32>& InTokens)
{
	GenThread->SetTokens(InTokens);
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

			int32* newDecodedTokens = nullptr;
			int32 newDecodedTokensSize = 0;
			tokenizer_decodeToken(GenThread->GetTok().GetTokenizer(), NewToken, &newDecodedTokens, &newDecodedTokensSize);

			if (newDecodedTokensSize == 0)
			{
				EncodedTokensSection.Unlock();
				return;
			}

			//UE_LOG(LogTemp, Warning, TEXT("---"), NewToken);
			for (int32 i = 0; i < newDecodedTokensSize; i++)
			{
				const char* str = GenThread->GetTok().DecodedTokenToString(newDecodedTokens[i]);
				//UE_LOG(LogTemp, Warning, TEXT("%s"), ANSI_TO_TCHAR(str));
			}

			const FTokenizer& Tok = GenThread->GetTok();

			//UE_LOG(LogTemp, Warning, TEXT("Pitch : %d"), NewToken);
			int32 LastToken = newDecodedTokens[newDecodedTokensSize - 1];

			int32 noteSequenceIndex = 0;
			TArray<RangeGroupHandle> rangeGroups;
			rangeGroups.Add(PitchRangeGroup);
			if (Tok.UseVelocities())
				rangeGroups.Add(VelocityRangeGroup);
			if (Tok.UseDuration())
				rangeGroups.Add(DurationRangeGroup);
			//if (FString("TSD") == Tok.GetTokenizationType())
			//	rangeGroups.Add(TimeShiftRangeGroup);

			if (noteSequenceIndex == 0)
			{
				if (Tok.IsPitch(LastToken))
				{
					++noteSequenceIndex;
				}
			}
			else
			{
				++noteSequenceIndex;
			}
			noteSequenceIndex = noteSequenceIndex % rangeGroups.Num();
			CurrentRangeGroup = rangeGroups[noteSequenceIndex];

			//if (Tok.IsPitch(LastToken))
			//{
			//	//UE_LOG(LogTemp, Warning, TEXT("Last Is Pitch"));
			//	CurrentRangeGroup = VelocityRangeGroup;
			//	//UE_LOG(LogTemp, Warning, TEXT("VelocityGroup"));
			//}
			//else if (Tok.IsVelocity(LastToken))
			//{
			//	//UE_LOG(LogTemp, Warning, TEXT("Last Is Velocity"));
			//	CurrentRangeGroup = DurationRangeGroup;
			//	//UE_LOG(LogTemp, Warning, TEXT("DurationGroup"));
			//}
			//else if (Tok.IsDuration(LastToken))
			//{
			//	//UE_LOG(LogTemp, Warning, TEXT("Last Is Other"));
			//	CurrentRangeGroup = PitchRangeGroup;
			//	//UE_LOG(LogTemp, Warning, TEXT("PitchGroup"));
			//}

			tokenizer_decodeToken_free(newDecodedTokens);

			EncodedTokensSection.Unlock();
		}));

		GenThread->SetOnInit(FOnInit::CreateLambda([this]()
		{
			SetFilter();
		}));

		// Set default tokens
		GenThread->GetEncodedTokens(NewEncodedTokens);

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
	const FTokenizer& tok2 = GenThread->GetTok();

	MidiTokenizerHandle tok = tok2.GetTokenizer();

	if (tok == nullptr)
	{
		return;
	}

	BaseRangeGroup = createRangeGroup();
	tokenizer_addTokensStartingByPosition(tok, BaseRangeGroup);
	tokenizer_addTokensStartingByBarNone(tok, BaseRangeGroup);
	tokenizer_addTokensStartingByTimeShift(tok, BaseRangeGroup);
	//rangeGroupUpdateCache(BaseRangeGroup);

	PitchRangeGroup = cloneRangeGroup(BaseRangeGroup);
	tokenizer_addTokensStartingByPitch(tok, PitchRangeGroup);
	rangeGroupUpdateCache(PitchRangeGroup);

	VelocityRangeGroup = createRangeGroup();// cloneRangeGroup(BaseRangeGroup);
	tokenizer_addTokensStartingByVelocity(tok, VelocityRangeGroup);
	rangeGroupUpdateCache(VelocityRangeGroup);

	DurationRangeGroup = createRangeGroup(); //cloneRangeGroup(BaseRangeGroup);
	tokenizer_addTokensStartingByDuration(tok, DurationRangeGroup);
	rangeGroupUpdateCache(DurationRangeGroup);

	TimeShiftRangeGroup = createRangeGroup(); //cloneRangeGroup(BaseRangeGroup);
	tokenizer_addTokensStartingByTimeShift(tok, TimeShiftRangeGroup);
	rangeGroupUpdateCache(TimeShiftRangeGroup);

	//AllRangeGroup = cloneRangeGroup(BaseRangeGroup);
	//tokenizer_addTokensStartingByPitch(tok, AllRangeGroup);
	//tokenizer_addTokensStartingByVelocity(tok, AllRangeGroup);
	//tokenizer_addTokensStartingByDuration(tok, AllRangeGroup);
	//tokenizer_addTokensStartingByTimeShift(tok, AllRangeGroup);
	//rangeGroupUpdateCache(AllRangeGroup);

	//CurrentRangeGroup = PitchRangeGroup;
	CurrentRangeGroup = PitchRangeGroup; // we don't know what was the last token / @TODO : set according to the tokens set by the user at the start






	GenThread->SetSearchStrategy(FOnSearch::CreateLambda([this](const SearchArgs& args)
		{
			//MidiTokenizerHandle tok = GenThread->GetTok();

			const Range* ranges;
			size_t nbRanges;
			rangeGroupGetRanges(CurrentRangeGroup, &ranges, &nbRanges);

			//temperatureTransform(&args, ranges, nbRanges);
			//repetitionPenalty(&args, ranges, nbRanges);
			args.logitsTensor[0] = -10000000;

			//float temperature = 1.1;
			//temperatureTransform(args.logitsTensor, ranges, nbRanges, temperature);

			float repetitionPenalty = 1.1;
			IAutoRegressivePipeline* Pipeline = GenThread->GetPipeline();
			GenerationHistory* History = Pipeline->getHistory(GenThread->GetBatch());
			//repetitionPenaltyTransform(args.logitsTensor, ranges, nbRanges, repetitionPenalty, History, 100);

			rangeGroupUpdateCache(CurrentRangeGroup);

			{
				SCOPE_CYCLE_COUNTER(STAT_GenThread_LogitProcessing1);
				SpecialPenaltyTransformArgs sArgs;
				sArgs.pitchWindowSize = 20;
				sArgs.pitchMaxAdditivePenalty = 0.05;
				specialPenaltyTransform(args.logitsTensor, CurrentRangeGroup, History, &sArgs);
			}

			MidiTokenizerHandle Tok = GenThread->GetTok().GetTokenizer();

			{
				SCOPE_CYCLE_COUNTER(STAT_GenThread_LogitProcessing2);
				musicalScalePenaltyTransform(args.logitsTensor, CurrentRangeGroup, Scales::Ionian::CMajor::get(), Scales::Ionian::CMajor::size(), 1.05, Tok);
			}
			{
				SCOPE_CYCLE_COUNTER(STAT_GenThread_LogitProcessing3);
				pitchRangePenaltyTransform(args.logitsTensor, CurrentRangeGroup, 40, 80, 0.05, Tok);
			}

			check(args.nbBatches == 1);
			int nbTopTokenSize = 40;
			size_t CurrentRangeGroupSize = rangeGroupSize(CurrentRangeGroup);
			TArray<int32> LogitIndices;
			LogitIndices.SetNumUninitialized(CurrentRangeGroupSize);
			int32* LogitIndicesData = LogitIndices.GetData();
			{
				SCOPE_CYCLE_COUNTER(STAT_GenThread_LogitProcessing4);
				rangeGroupWrite(CurrentRangeGroup, LogitIndicesData);
			}
			check(CurrentRangeGroupSize >= nbTopTokenSize);
			{
				SCOPE_CYCLE_COUNTER(STAT_GenThread_LogitProcessing5);
				sortLogits(args.logitsTensor, LogitIndicesData, LogitIndicesData + CurrentRangeGroupSize, nbTopTokenSize);
			}

			{
				SCOPE_CYCLE_COUNTER(STAT_GenThread_LogitProcessing6);
				stableSoftmax(args.logitsTensor, LogitIndicesData, LogitIndicesData + nbTopTokenSize);
			}

			{
				SCOPE_CYCLE_COUNTER(STAT_GenThread_LogitProcessing7);
				int outToken = topPSampling(args.logitsTensor, LogitIndicesData, LogitIndicesData + nbTopTokenSize, 0.5);
				args.outNextTokens[0] = outToken;
			}
		}));
}

void FMIDIGeneratorEnv::DecodeTokens()
{
	struct Args
	{
		FMIDIGeneratorEnv* self;

		int32 LastTick = 0;
	};

	Args args{ this };

	if (converter == nullptr)
		converter = createTSDConverter();

	converterSetTokenizer(converter, GenThread->GetTok().GetTokenizer());

	converterSetOnNote(converter, [](void* data, const Note& newNote)
		{
			Args& args = *(Args*)(data);

			int32 Channel = 1;
			int32 NoteNumber = newNote.pitch;
			int32 Velocity = newNote.velocity;

			int32 Track = 0;

			//int32 CurrentTick = 0;// args.self->Outputs.MidiClock->GetCurrentHiResTick();
			int32 CurrentTick = args.self->CurrentTick;// args.self->Outputs.MidiClock->GetCurrentHiResTick();

			int32 Tick = newNote.tick * 100 + args.self->AddedTicks;

			if (Tick < CurrentTick)
			{
				args.self->AddedTicks += CurrentTick - Tick;
				Tick = CurrentTick;
				UE_LOG(LogTemp, Warning, TEXT("Tick < CurrentTick: %d < %d"), CurrentTick, Tick);
			}

			if (args.self->MidiFileData->Tracks[0].GetUnsortedEvents().IsEmpty() or Tick >= args.self->MidiFileData->Tracks[0].GetEvents().Last().GetTick())
			{
				args.LastTick = Tick;

				FMidiMsg Msg{ FMidiMsg::CreateNoteOn(Channel - 1, NoteNumber, Velocity) };
				args.self->MidiFileData->Tracks[0].AddEvent(FMidiEvent(Tick, Msg));

				//int32 Tick2 = (newNote.tick+50) * 100 + args.self->AddedTicks;
				//FMidiMsg Msg2{ FMidiMsg::CreateNoteOff(Channel - 1, NoteNumber) };
				//args.self->MidiFileData->Tracks[0].AddEvent(FMidiEvent(Tick2, Msg2));

				//args.self->MidiFileData->GetLastEventTick();
			}

		});

	int32* newDecodedTokens = nullptr;
	int32 newDecodedTokensSize = 0;
	EncodedTokensSection.Lock();
	tokenizer_decodeIDs(GenThread->GetTok().GetTokenizer(), NewEncodedTokens.GetData(), NewEncodedTokens.Num(), &newDecodedTokens, &newDecodedTokensSize);
	NewEncodedTokens.Empty();
	EncodedTokensSection.Unlock();
	for (int32 newDecodedTokenIndex = 0; newDecodedTokenIndex < newDecodedTokensSize; newDecodedTokenIndex++)
	{
		DecodedTokens.Add(newDecodedTokens[newDecodedTokenIndex]);
	}

	tokenizer_decodeIDs_free(newDecodedTokens);

	std::int32_t i = nextTokenToProcess;
	while (i < DecodedTokens.Num())
	{

		bool isSuccess = converterProcessToken(converter, DecodedTokens.GetData(), DecodedTokens.Num(), &i, &args);
		if (isSuccess)
		{
			nextTokenToProcess = i;
		}
		else
		{
			i++; // ignore current token and continue
			if (i - nextTokenToProcess > 20)
			{
				i += 10; // in case there are too many errors, ignore the 10 next tokens
				nbSkips++;
				UE_LOG(LogTemp, Warning, TEXT("Skips: %d"), nbSkips);
			}
		}
	}
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

void UMIDIGeneratorEnv::PreloadPipeline(const FString& ModelPath)
{
	Generator->MidiGenerator->PreloadPipeline(ModelPath);
}

void UMIDIGeneratorEnv::SetFilter()
{
	Generator->MidiGenerator->SetFilter();
}

void UMIDIGeneratorEnv::SetTokenizer(UTokenizerAsset* InTokenizer)
{
	Generator->MidiGenerator->GenThread->SetTok(InTokenizer->GetTokenizerPtr());
}

void UMIDIGeneratorEnv::SetTokens(const TArray<int32>& InTokens)
{
	Generator->MidiGenerator->SetTokens(InTokens);
}

TSharedPtr<Audio::IProxyData> UMIDIGeneratorEnv::CreateProxyData(const Audio::FProxyDataInitParams& InitParams)
{
	return MakeShared<FMIDIGeneratorProxy, ESPMode::ThreadSafe>(this);
}