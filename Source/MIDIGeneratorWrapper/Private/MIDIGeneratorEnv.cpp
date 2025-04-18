// Fill out your copyright notice in the Description page of Project Settings.


#include "MIDIGeneratorEnv.h"
#include "GenThread.h"
#include "HarmonixMetasound/DataTypes/MidiClock.h"
#include "HarmonixMetasound/DataTypes/MusicTimeInterval.h"

#include "modelBuilderManager.hpp"
#include "abstractPipeline.hpp"
#include "logitProcessing.h"
#include "logitProcessing.hpp"
#include "searchArgs.h"
#include "generationHistory.h"
#include "onAddTokensArgs.hpp"
#include "midiConverter.h"

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

void FMIDIGeneratorEnv::UpdateCurrentRangeGroup(int32 LastDecodedToken)
{
	const FTokenizer& Tok = GenThread->GetTok();

	if (Tok.IsTimeShift(LastDecodedToken))
	{
		CurrentRangeGroup = PitchRangeGroup;
		return;
	}

	int32 noteSequenceIndex = 0;
	TArray<RangeGroupHandle> rangeGroups;
	//if (nbEncodedTokensSinceRegen < 3)
	//{
	//	//hasRegenCounter++;
	//	rangeGroups.Add(PitchRangeGroup);
	//	//if (hasRegenCounter > 2)
	//	//hasRegen = false;
	//}
	//else
		rangeGroups.Add(PitchTimeshiftRangeGroup);
	if (Tok.UseVelocities())
		rangeGroups.Add(VelocityRangeGroup);
	if (Tok.UseDuration())
			rangeGroups.Add(DurationRangeGroup);
	//if (FString("TSD") == Tok.GetTokenizationType())
	//	rangeGroups.Add(TimeShiftRangeGroup);

	if (noteSequenceIndex == 0)
	{
		if (Tok.IsPitch(LastDecodedToken))
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
				nbEncodedTokensSinceRegen++;
				bShouldUpdateTokens = true;
				NewEncodedTokens.Add(NewToken);

				const int32_t* outDecodedTokensBegin;
				const int32_t* outDecodedTokensEnd;
				tokenizer_decodeTokenFast(GenThread->GetTok().GetTokenizer(), NewToken, &outDecodedTokensBegin, &outDecodedTokensEnd);

				if (outDecodedTokensEnd - outDecodedTokensBegin == 0)
				{
					return;
				}

				////UE_LOG(LogTemp, Warning, TEXT("---"), NewToken);
				//for (int32 i = 0; i < newDecodedTokensSize; i++)
				//{
				//	const char* str = GenThread->GetTok().DecodedTokenToString(newDecodedTokens[i]);
				//	//UE_LOG(LogTemp, Warning, TEXT("%s"), ANSI_TO_TCHAR(str));
				//}

				//UE_LOG(LogTemp, Warning, TEXT("Pitch : %d"), NewToken);
				int32 LastToken = *(outDecodedTokensEnd - 1);

				UpdateCurrentRangeGroup(LastToken);
			}));

		GenThread->SetOnInit(FOnInit::CreateLambda([this]()
			{
				SetFilter();
			}));

		GenThread->OnCacheRemoved.BindLambda([this](int32 libTick)
			{
				GenerationHistory* History = GenThread->GetPipeline()->getHistory(GenThread->GetBatch());
				const struct Note* OutNotes;
				size_t OutLength;
				generationHistory_getNotes(History, &OutNotes, &OutLength);

				nextNoteIndexToProcess = FMath::Min(nextNoteIndexToProcess, int32(OutLength));

				TokenHistoryHandle decodedTokenHistory = getDecodedTokensHistory(History);
				const int32* decodedTokens;
				int32 decodedTokensSize;
				tokenHistory_getTokens(decodedTokenHistory, &decodedTokens, &decodedTokensSize);
				UpdateCurrentRangeGroup(decodedTokens[decodedTokensSize - 1]);


				GenThread->GetPipeline()->setSequencerUserData(GenThread->GetBatch(), this);
				GenThread->GetPipeline()->addCallbackToSequencer(GenThread->GetBatch(), libTick, [](int32_t genLibTick, void* userData)
					{
						FMIDIGeneratorEnv& env = *(FMIDIGeneratorEnv*)userData;

						env.hasRegen = true;
						env.nbEncodedTokensSinceRegen = 0;
						env.nbAddedSinceLastTimeshift = 0;
						env.nbAddedSinceLast = 0;



						GenerationHistory* History = env.GenThread->GetPipeline()->getHistory(env.GenThread->GetBatch());

						Note* outNotes;
						size_t outLength;
						generationHistory_getNotesMut(History, &outNotes, &outLength);

						//Note newNote;
						//newNote = outNotes[outLength - 1];
						//newNote.tick = genLibTick;
						//newNote.pitch = 90;
						//generationHistory_addStandaloneNote(History, &newNote);
						//newNote.pitch = 40;
						//generationHistory_addStandaloneNote(History, &newNote);

						Note newNote;
						newNote = outNotes[outLength - 1];
						newNote.tick = genLibTick;
						generationHistory_addStandaloneNote(History, &newNote);
						newNote.pitch += 4;
						generationHistory_addStandaloneNote(History, &newNote);
						newNote.pitch += 3;
						generationHistory_addStandaloneNote(History, &newNote);
					});

				//generationHistory_setOnNoteAddedData(History, this);
				//generationHistory_setOnNoteAdded(History, [](void* userData)
				//	{
				//		FMIDIGeneratorEnv& env = *(FMIDIGeneratorEnv*)userData;

				//		GenerationHistory* History = env.GenThread->GetPipeline()->getHistory(env.GenThread->GetBatch());

				//		Note* outNotes;
				//		size_t outLength;
				//		generationHistory_getNotesMut(History, &outNotes, &outLength);

				//		Note newNote;
				//		newNote = outNotes[outLength - 1];
				//		newNote.pitch = 90;
				//		generationHistory_addStandaloneNote(History, &newNote);
				//		newNote.pitch = 40;
				//		generationHistory_addStandaloneNote(History, &newNote);

				//		generationHistory_setOnNoteAddedData(History, nullptr);
				//		generationHistory_setOnNoteAdded(History, nullptr);
				//	});
			});

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

	PitchTimeshiftRangeGroup = cloneRangeGroup(BaseRangeGroup);
	tokenizer_addTokensStartingByPitch(tok, PitchTimeshiftRangeGroup);
	rangeGroupUpdateCache(PitchTimeshiftRangeGroup);

	PitchRangeGroup = createRangeGroup();
	tokenizer_addTokensStartingByPitch(tok, PitchRangeGroup);
	rangeGroupUpdateCache(PitchRangeGroup);

	VelocityRangeGroup = createRangeGroup();
	tokenizer_addTokensStartingByVelocity(tok, VelocityRangeGroup);
	rangeGroupUpdateCache(VelocityRangeGroup);

	DurationRangeGroup = createRangeGroup();
	tokenizer_addTokensStartingByDuration(tok, DurationRangeGroup);
	rangeGroupUpdateCache(DurationRangeGroup);

	TimeShiftRangeGroup = createRangeGroup();
	tokenizer_addTokensStartingByTimeShift(tok, TimeShiftRangeGroup);
	rangeGroupUpdateCache(TimeShiftRangeGroup);

	AllRangeGroup = cloneRangeGroup(BaseRangeGroup);
	tokenizer_addTokensStartingByPitch(tok, AllRangeGroup);
	tokenizer_addTokensStartingByVelocity(tok, AllRangeGroup);
	tokenizer_addTokensStartingByDuration(tok, AllRangeGroup);
	tokenizer_addTokensStartingByTimeShift(tok, AllRangeGroup);
	tokenizer_addTokensStartingByPosition(tok, AllRangeGroup);
	tokenizer_addTokensStartingByBarNone(tok, AllRangeGroup);
	rangeGroupUpdateCache(AllRangeGroup);

	//CurrentRangeGroup = PitchTimeshiftRangeGroup;
	CurrentRangeGroup = PitchTimeshiftRangeGroup; // we don't know what was the last token / @TODO : set according to the tokens set by the user at the start






	GenThread->SetSearchStrategy(FOnSearch::CreateLambda([this](const SearchArgs& args)
		{
			//MidiTokenizerHandle tok = GenThread->GetTok();

			//CurrentRangeGroup = AllRangeGroup;

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

			//{
			//	SCOPE_CYCLE_COUNTER(STAT_GenThread_LogitProcessing1);
			//	SpecialPenaltyTransformArgs sArgs;
			//	sArgs.pitchWindowSize = 20;
			//	sArgs.pitchMaxAdditivePenalty = 0.05;
			//	specialPenaltyTransform(args.logitsTensor, CurrentRangeGroup, History, &sArgs);
			//}

			MidiTokenizerHandle Tok = GenThread->GetTok().GetTokenizer();

			{
				SCOPE_CYCLE_COUNTER(STAT_GenThread_LogitProcessing2);
				//if (hasRegen)
				//{
				//	int32 scale[] = {0};
				//	musicalScalePenaltyTransform(args.logitsTensor, CurrentRangeGroup, scale, 1, 1.05, Tok);
				//}
				//else
				{
					musicalScalePenaltyTransform(args.logitsTensor, CurrentRangeGroup, Scales::Ionian::CMajor::get(), Scales::Ionian::CMajor::size(), 1.05, Tok);
				}
			}
			{
				SCOPE_CYCLE_COUNTER(STAT_GenThread_LogitProcessing3);
				//if (nbEncodedTokensSinceRegen < 1)
				//{
				//	//pitchRangePenaltyTransform(args.logitsTensor, CurrentRangeGroup, 70, 90, 0.7, Tok);
				//	pitchRangePenaltyTransform(args.logitsTensor, CurrentRangeGroup, 70, 90, 15.0, Tok);
				//}
				//else
				{
					pitchRangePenaltyTransform(args.logitsTensor, CurrentRangeGroup, 40, 60, 0.7, Tok);
				}
				//pitchRangePenaltyTransform(args.logitsTensor, CurrentRangeGroup, 40, 80, 0.05, Tok);
			}

			{
				//SCOPE_CYCLE_COUNTER(STAT_GenThread_LogitProcessing3);
				timeShiftRangePenaltyTransform(args.logitsTensor, CurrentRangeGroup, 0.0, 2.0, 1.05, Tok);

				//if (nbEncodedTokensSinceRegen < 20)
				//{
				//	timeShiftRangePenaltyTransform(args.logitsTensor, CurrentRangeGroup, -1.0, -1, 1.05, Tok);
				//}
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
double MaxTime = 0.0;
int nTimes = 0;
void FMIDIGeneratorEnv::DecodeTokens()
{
	struct Args
	{
		FMIDIGeneratorEnv* self;

		int32 LastTick = 0;
	};

	Args args{ this };
	 
	auto onNote = [](void* data, const Note& newNote)
	{
		Args& args = *(Args*)(data);

		int32 Channel = 1;
		int32 NoteNumber = newNote.pitch;
		int32 Velocity = newNote.velocity;
		if (NoteNumber < 60)
		{
			Velocity += 60 - NoteNumber;
		}

		int32 Track = 0;

		//int32 CurrentTick = 0;// args.self->Outputs.MidiClock->GetCurrentHiResTick();
		int32 CurrentTick = args.self->CurrentTick;// args.self->Outputs.MidiClock->GetCurrentHiResTick();

		int32 Tick = FMath::RoundToInt32(float(args.self->GenLibTickToUETick(newNote.tick)));

		if (Tick < CurrentTick)
		{
			args.self->AddedTicks += CurrentTick - Tick;
			Tick = CurrentTick;
			//UE_LOG(LogTemp, Warning, TEXT("Tick < CurrentTick: %d < %d"), CurrentTick, Tick);
		}

		//if (args.self->regenTick < 3)
		//{
		//	Tick = args.self->CacheRemoveTick;
		//}

		if (args.self->MidiFileData->Tracks[0].GetUnsortedEvents().IsEmpty() or Tick >= args.self->MidiFileData->Tracks[0].GetEvents().Last().GetTick())
		{
			//UE_LOG(LogTemp, Warning, TEXT("New Note Tick : %d / CurrentTick : %d"), Tick, CurrentTick);
			args.LastTick = Tick;

			//if (args.self->regenTick < 10)
			//{
			//	Velocity = 120;
			//}
			FMidiMsg Msg{ FMidiMsg::CreateNoteOn(Channel - 1, NoteNumber, Velocity) }; 
			args.self->MidiFileData->Tracks[0].AddEvent(FMidiEvent(Tick, Msg));

			//args.self->regenTick++;

			//int32 Tick2 = (newNote.tick+50) * 100 + args.self->AddedTicks;
			//FMidiMsg Msg2{ FMidiMsg::CreateNoteOff(Channel - 1, NoteNumber) };
			//args.self->MidiFileData->Tracks[0].AddEvent(FMidiEvent(Tick2, Msg2));

			//args.self->MidiFileData->GetLastEventTick();
		}

	};

	//const double StartTime = FPlatformTime::Seconds();

	GenerationHistory* History = GenThread->GetPipeline()->getHistory(GenThread->GetBatch());
	//generationHistory_convert(History);

	const struct Note* OutNotes;
	size_t OutLength;
	generationHistory_getNotes(History, &OutNotes, &OutLength);

	{
		while (nextNoteIndexToProcess < OutLength)
		{
			onNote(&args, OutNotes[nextNoteIndexToProcess]);
			nextNoteIndexToProcess++;
		}

	}

	//const double EndTime = FPlatformTime::Seconds();
	//const double Elapsed = EndTime - StartTime;

	//if (nTimes > 500)
	//MaxTime = FMath::Max(MaxTime, Elapsed);
	//nTimes++;

	//UE_LOG(LogTemp, Warning, TEXT("Max: %f"), MaxTime);

	float CurrentTimeMs = Clock->GetCurrentHiResMs();
	float Tick = MidiFileData->SongMaps.GetTempoMap().MsToTick(CurrentTimeMs);
	int32 genLibTick = UETickToGenLibTick(Tick);

	//UE_LOG(LogTemp, Warning, TEXT("Clock UE: %d -> Lib: %d"), int32(Tick), genLibTick);
	//UE_LOG(LogTemp, Warning, TEXT("LastNote Lib: %d -> UE: %f"), OutNotes[OutLength-1].tick, GenLibTickToUETick(OutNotes[OutLength - 1].tick));
	//UE_LOG(LogTemp, Warning, TEXT("---"));

	GenThread->CurrentTick.Set(genLibTick);

	// @TODO : thread safe
	if (GenThread->ShouldResumeGeneration())
	{
		GenThread->Semaphore->Trigger();
	}
}

void FMIDIGeneratorEnv::SetClock(const HarmonixMetasound::FMidiClock& InClock)
{
	Clock = &InClock;
}

int32 FMIDIGeneratorEnv::UETickToGenLibTick(float tick)
{
	return int32((tick - AddedTicks) / 100);
}

float FMIDIGeneratorEnv::GenLibTickToUETick(int32 tick)
{
	return tick * 100 + AddedTicks;
}

void FMIDIGeneratorEnv::RegenerateCacheAfterDelay(float DelayInMs)
{
	if (Clock == nullptr)
	{
		return;
	}

	{
		//regenTick = 0;
		//regenNbPitches = 0;
		//hasRegen = true;
		//nbEncodedTokensSinceRegen = 0;
		//nbAddedSinceLastTimeshift = 0;
		//nbAddedSinceLast = 0;
		GenerationHistory* History = GenThread->GetPipeline()->getHistory(GenThread->GetBatch());

		//MidiTokenizerHandle Tok = GenThread->GetTok().GetTokenizer();
		//pitch1 = findPitchToken(Tok, 58);
		//pitch2 = findPitchToken(Tok, 60);
		//pitch3 = findPitchToken(Tok, 62);

		//generationHistory_setOnNoteAddedData(History, this);
		//generationHistory_setOnNoteAdded(History, [](void* userData)
		//{
		//	FMIDIGeneratorEnv& env = * (FMIDIGeneratorEnv*)userData;

		//	GenerationHistory* History = env.GenThread->GetPipeline()->getHistory(env.GenThread->GetBatch());

		//	Note* outNotes;
		//	size_t outLength;
		//	generationHistory_getNotesMut(History, &outNotes, &outLength);

		//	Note newNote;
		//	newNote = outNotes[outLength - 1];
		//	newNote.pitch = 90;
		//	generationHistory_addStandaloneNote(History, &newNote);
		//	newNote.pitch = 40;
		//	generationHistory_addStandaloneNote(History, &newNote);

		//	generationHistory_setOnNoteAddedData(History, nullptr);
		//	generationHistory_setOnNoteAdded(History, nullptr);
		//});

		//generationHistory_setOnEncodedTokenAddedData(History, this);
		//generationHistory_setOnEncodedTokenAdded(History, [](OnAddTokensArgs* args)
		//{
		//	const MidiTokenizer& tokenizer = args->getTokenizer();
		//	FMIDIGeneratorEnv& env = args->getUserData<FMIDIGeneratorEnv>();

		//	//args->addDecodedToken(env.pitch1);
		//	//args->addDecodedToken(env.pitch2);
		//	//args->addDecodedToken(env.pitch3);

		//	const int32_t* outDecodedTokensBegin;
		//	const int32_t* outDecodedTokensEnd;
		//	tokenizer_decodeTokenFast(&tokenizer, args->getNewEncodedToken(), &outDecodedTokensBegin, &outDecodedTokensEnd);

		//	while (outDecodedTokensBegin != outDecodedTokensEnd)
		//	{
		//		int32 decodedToken = *outDecodedTokensBegin;

		//		//if (isPitch(&tokenizer, decodedToken))
		//		//{
		//		//	//env.regenNbPitches++;

		//		//	int32 pitch = getPitch(&tokenizer, decodedToken);

		//		//	// Triad
		//		//	args->addDecodedToken(findPitchToken(&tokenizer, 40));
		//		//	//args->addDecodedToken(findPitchToken(&tokenizer, pitch + 4));
		//		//	//args->addDecodedToken(findPitchToken(&tokenizer, pitch + 7));
		//		//	//args->addDecodedToken(findPitchToken(&tokenizer, pitch + 2));
		//		//	//args->addDecodedToken(findPitchToken(&tokenizer, pitch + 9));
		//		//	//args->addDecodedToken(findPitchToken(&tokenizer, pitch + 9));
		//		//	//args->addDecodedToken(findPitchToken(&tokenizer, 80));
		//		//	//args->addDecodedToken(findPitchToken(&tokenizer, 90));
		//		//	//args->addDecodedToken(findPitchToken(&tokenizer, 50));
		//		//}

		//		if (env.nbAddedSinceLastTimeshift == 0)
		//		{
		//			args->addDecodedToken(findPitchToken(&tokenizer, 40));
		//			env.nbAddedSinceLastTimeshift++;
		//			env.nbAddedSinceLast++;
		//		}

		//		if (isTimeShift(&tokenizer, decodedToken))
		//		{
		//			env.nbAddedSinceLastTimeshift = 0;
		//		}
		//		//{
		//		//	args->addDecodedToken(*outDecodedTokensBegin);
		//		//}
		//		args->addDecodedToken(*outDecodedTokensBegin);
		//		//	args->addDecodedToken(findPitchToken(&tokenizer, 90));
		//		//	args->addDecodedToken(*outDecodedTokensBegin);
		//		//	args->addDecodedToken(findPitchToken(&tokenizer, 90));
		//		//	args->addDecodedToken(*outDecodedTokensBegin);
		//		//}

		//		++outDecodedTokensBegin;
		//	}

		//	//if (env.regenNbPitches < 5)
		//	//{
		//	//	return;
		//	//}
		//	if (env.nbAddedSinceLast < 3)
		//	{
		//		return;
		//	}
		//	GenerationHistory* History = env.GenThread->GetPipeline()->getHistory(env.GenThread->GetBatch());
		//	generationHistory_setOnEncodedTokenAdded(History, generationHistory_getDefaultOnEncodedTokenAdded());
		//});
	}

	float CurrentTimeMs = Clock->GetCurrentHiResMs();
	float Tick = MidiFileData->SongMaps.GetTempoMap().MsToTick(CurrentTimeMs + DelayInMs);
	CacheRemoveTick = Tick;
	int32 genLibTick = UETickToGenLibTick(Tick);




	GenThread->RemoveCacheAfterTick(genLibTick);

	Clock->GetDrivingMidiPlayCursorMgr()->LockForMidiDataChanges();
	MidiFileData->Tracks[0].ClearEventsAfter(int32(Tick), true);
	Clock->GetDrivingMidiPlayCursorMgr()->MidiDataChangeComplete(FMidiPlayCursorMgr::EMidiChangePositionCorrectMode::MaintainTick);
	
	//GenerationHistory* History = GenThread->GetPipeline()->getHistory(GenThread->GetBatch());

	//GenThread->GenerationMutex.Lock();
	//GenThread->shouldIgnoreNextToken = true;
	//generationHistory_removeAfterTick(History, genLibTick);

	//const struct Note* OutNotes;
	//size_t OutLength;
	//generationHistory_getNotes(History, &OutNotes, &OutLength);

	//nextNoteIndexToProcess = FMath::Min(nextNoteIndexToProcess, int32(OutLength));

	//TokenHistoryHandle decodedTokenHistory = getDecodedTokensHistory(History);
	//const int32* decodedTokens;
	//int32 decodedTokensSize;
	//tokenHistory_getTokens(decodedTokenHistory, &decodedTokens, &decodedTokensSize);
	//UpdateCurrentRangeGroup(decodedTokens[decodedTokensSize - 1]);

	//MidiFileData->Tracks[0].ClearEventsAfter(int32(Tick), true);
	//GenThread->GenerationMutex.Unlock();
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


void UMIDIGeneratorEnv::SetTempo(float InTempo)
{
	const HarmonixMetasound::FMidiClock* Clock = Generator->MidiGenerator->Clock;
	if (Clock == nullptr)
	{
		return;
	}
	int32 CurrentTick = Clock->GetCurrentMidiTick();

	Clock->GetDrivingMidiPlayCursorMgr()->LockForMidiDataChanges();
	TSharedPtr<struct FMidiFileData>& MidiFileData = Generator->MidiGenerator->MidiFileData;
	MidiFileData->AddTempoChange(0, CurrentTick, InTempo);
	Clock->GetDrivingMidiPlayCursorMgr()->MidiDataChangeComplete(FMidiPlayCursorMgr::EMidiChangePositionCorrectMode::MaintainTick);

	//Clock.time

	//MidiFileData->SongMaps.GetTempoMap().MsToTick()



	//->AddTempoChange() = InTempo;

	//int32 CurrentTempo = 120;
	//int32 CurrentTimeSigNum = 4;
	//int32 CurrentTimeSigDenom = 4;
	//MidiFileData = HarmonixMetasound::FMidiClock::MakeClockConductorMidiData(CurrentTempo, CurrentTimeSigNum, CurrentTimeSigDenom);

	//MidiFileData->MidiFileName = "Generated";
	//MidiFileData->TicksPerQuarterNote = 16;
}

void UMIDIGeneratorEnv::RegenerateCacheAfterDelay(float DelayInMs)
{
	Generator->MidiGenerator->RegenerateCacheAfterDelay(DelayInMs);
}


TSharedPtr<Audio::IProxyData> UMIDIGeneratorEnv::CreateProxyData(const Audio::FProxyDataInitParams& InitParams)
{
	return MakeShared<FMIDIGeneratorProxy, ESPMode::ThreadSafe>(this);
}