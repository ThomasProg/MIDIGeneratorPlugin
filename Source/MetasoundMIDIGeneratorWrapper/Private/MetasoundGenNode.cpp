// Copyright Epic Games, Inc. All Rights Reserved.

#include "MetasoundGenNode.h"
#include "HarmonixMetasound/DataTypes/MidiStream.h"
#include "Interfaces/MetasoundFrontendSourceInterface.h"
#include "MetasoundDataFactory.h"
#include "MetasoundEngineNodesNames.h"
#include "MetasoundExecutableOperator.h"
#include "MetasoundFacade.h"
#include "MetasoundNodeRegistrationMacro.h"
#include "MetasoundPrimitives.h"
#include "MetasoundStandardNodesCategories.h"
#include "MetasoundStandardNodesNames.h"
#include "MetasoundTrace.h"
#include "MetasoundTrigger.h"
#include <DSP/MultichannelBuffer.h>
#include "Engine/Engine.h"

#include "MetasoundExecutableOperator.h"     // TExecutableOperator class
#include "MetasoundPrimitives.h"             // ReadRef and WriteRef descriptions for bool, int32, float, and string
#include "MetasoundNodeRegistrationMacro.h"  // METASOUND_LOCTEXT and METASOUND_REGISTER_NODE macros
#include "MetasoundStandardNodesNames.h"     // StandardNodes namespace
#include "MetasoundFacade.h"				         // FNodeFacade class, eliminates the need for a fair amount of boilerplate code
#include "MetasoundParamHelper.h"            // METASOUND_PARAM and METASOUND_GET_PARAM family of macros
#include "HarmonixMetasound/DataTypes/MusicTransport.h"

#include "HarmonixMetasound/DataTypes/MidiClock.h"
#include "HarmonixMetasound/DataTypes/MusicTimeInterval.h"
#include "MIDIGenerator.h"
#include "GenThread.h"

// Required for ensuring the node is supported by all languages in engine. Must be unique per MetaSound.
#define LOCTEXT_NAMESPACE "MetasoundStandardNodes_MetaSoundAIGenNode"



namespace AIGen
{
	namespace Inputs
	{
		METASOUND_PARAM(Transport, "Transport", "The synthesized right audio channel.");
		METASOUND_PARAM(Clock, "Clock", "The Clock having the timing of the notes.");
	}

	namespace Outputs
	{
		METASOUND_PARAM(MidiStream, "MidiStream", "MidiStream that is synthesized.");
		METASOUND_PARAM(MidiClock, "MidiClock", "MidiClock.");
	}
}


namespace Metasound
{
	// Based on FMidiPlayerOperator
	class FAIGenOperator : public TExecutableOperator<FAIGenOperator>, public FMidiVoiceGeneratorBase, public HarmonixMetasound::FMusicTransportControllable, public FMidiPlayCursor
	{
	public:
		// Constructor
		FAIGenOperator(const Metasound::FBuildOperatorParams& InParams,
			const HarmonixMetasound::FMidiClockReadRef& InMidiClock,
			const HarmonixMetasound::FMusicTransportEventStreamReadRef& InTransport)
			: FMusicTransportControllable(HarmonixMetasound::EMusicPlayerTransportState::Prepared)
			, Inputs{ InMidiClock, InTransport }
			, Outputs{ HarmonixMetasound::FMidiStreamWriteRef::CreateNew(), HarmonixMetasound::FMidiClockWriteRef::CreateNew(InParams.OperatorSettings) }
			, BlockSize(InParams.OperatorSettings.GetNumFramesPerBlock())
		{
			//TSharedPtr<FMyObjectType, ESPMode::ThreadSafe> NewThreadsafePointer = MakeShared<FMyObjectType, ESPMode::ThreadSafe>(MyArgs);
			//MidiFileData = MakeShared<FMidiFileData>();
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
			Outputs.MidiStream->SetMidiFile(MidiDataProxy);

			Outputs.MidiClock->RegisterHiResPlayCursor(this);
			Outputs.MidiStream->SetClock(*Outputs.MidiClock);
			//Outputs.MidiStream->SetTicksPerQuarterNote(1);
			

			if ((!bIsThreaded) && Generator == nullptr)
			{
				Generator = NewObject<UMIDIGenerator>();
			}

			if (bIsThreaded)
			{
				GenThread.OnGenerated.BindLambda([this]()
					{
						TokenModifSection.Lock();
						bShouldUpdateTokens = true;
						Tokens = GenThread.EncodedLine;
						TokenModifSection.Unlock();
					});
			}

			FString TokPath = "C:/Users/thoma/Documents/Unreal Projects/MIDITokCpp/tokenizer.json";
			FString ModelPath = "C:/Users/thoma/Documents/Unreal Projects/MIDITokCpp/onnx_model_path/gpt2-midi-model_past.onnx";
			int32 input_ids[] = {
			942,    65,  1579,  1842,   616,    46,  3032,  1507,   319,  1447,
			12384,  1016,  1877,   319, 15263,  3396,   302,  2667,  1807,  3388,
			2649,  1173,    50,   967,  1621,   256,  1564,   653,  1701,   377
			};
			int32 size = sizeof(input_ids) / sizeof(*input_ids);

			TArray<int32> EncodedTokens;
			EncodedTokens.SetNum(size);
			for (int i = 0; i < size; i++)
			{
				EncodedTokens[i] = input_ids[i];
			}

			GenThread.Start(TokPath, ModelPath, EncodedTokens);

			if (!bIsThreaded)
			{
				Generator->Init();
				Generator->Generate(120, Tokens);
			}

			TokenModifSection.Lock();
			Tokens = GenThread.EncodedLine;
			UpdateScheduledMidiEvents();
			TokenModifSection.Unlock();

			Reset(InParams);

			Outputs.MidiClock->AttachToMidiResource(MidiFileData);
		}

		TArray<HarmonixMetasound::FMidiStreamEvent> EventsToPlay;

		//int32 LastTick = 0;
		void UpdateScheduledMidiEvents()
		{
			Outputs.MidiClock->GetDrivingMidiPlayCursorMgr()->LockForMidiDataChanges();

			//std::int32_t i = UnplayedTokenIndex;
			std::int32_t i = 0;

			struct Args
			{
				std::int32_t& i;
				std::int32_t& unplayedTokenIndex;
				FAIGenOperator* self;

				int32 LastTick = 0;
			};

			Args args{ i, UnplayedTokenIndex, this/*, LastTick*/ };

			MidiConverterHandle converter = createMidiConverter();
			converterSetTokenizer(converter, GenThread.Generator.tok);
			//converterSetTokenizer(converter, Generator->gen.tok);

			//Outputs.MidiClock->LockForMidiDataChanges();

			int32 currentTick = Outputs.MidiClock->GetCurrentHiResTick();
			MidiFileData->Tracks[0].Empty();
			//MidiFileData->Tracks[0].ClearEventsAfter(currentTick, false);
			converterSetOnNote(converter, [](void* data, const Note& newNote)
				{
					Args& args = *(Args*)(data);
					args.unplayedTokenIndex = args.i + 1;

					//args.self->Cursor.p

					int32 Channel = 1;
					int32 NoteNumber = newNote.pitch;
					int32 Velocity = newNote.velocity;

					int32 Track = 0;
					//int32 Tick = newNote.tick*180;
					int32 Tick = newNote.tick * 100;

					if (args.LastTick <= Tick)
					{
						args.LastTick = Tick;

						FMidiMsg Msg{ FMidiMsg::CreateNoteOn(Channel - 1, NoteNumber, Velocity) };
						//HarmonixMetasound::FMidiStreamEvent Event{ &args.self->VoiceGenerator, Msg };
						//Event.BlockSampleFrameIndex = args.self->Outputs.MidiClock->GetCurrentBlockFrameIndex();;
						//Event.AuthoredMidiTick = Tick; //NextPulse.MidiTick;
						//Event.CurrentMidiTick = Tick; //NextPulse.MidiTick;
						//Event.TrackIndex = Track;
						//args.self->EventsToPlay.Push(Event);

						args.self->MidiFileData->Tracks[0].AddEvent(FMidiEvent(Tick, Msg));
						//args.self->Outputs.MidiStream->InsertMidiEvent(Event);
					}

				});

			int32* outTokens = nullptr;
			int32 outTokensSize = 0;
			tokenizer_decodeIDs(GenThread.Generator.tok, Tokens.GetData(), Tokens.Num(), &outTokens, &outTokensSize);

			while (i < outTokensSize)
			{
				converterProcessToken(converter, outTokens, outTokensSize, i, &args);
				i++;
			}

			ensureMsgf(!MidiFileData->Tracks[0].GetEvents().IsEmpty(), TEXT("Should not be empty!"));
			//checkf((Index >= 0)& (Index < ArrayNum), TEXT("Array index out of bounds: %lld into an array of size %lld"), (long long)Index, (long long)ArrayNum); // & for one branch

			tokenizer_decodeIDs_free(outTokens);

			//Outputs.MidiClock->MidiDataChangesComplete(FMidiPlayCursorMgr::EMidiChangePositionCorrectMode::MaintainTick);

			destroyMidiConverter(converter);
			MidiFileData->Tracks[0].Sort();
			//MidiFileData->Sort


			Outputs.MidiClock->GetDrivingMidiPlayCursorMgr()->MidiDataChangeComplete(FMidiPlayCursorMgr::EMidiChangePositionCorrectMode::MaintainTick);
		}

		void Reset(const FResetParams&)
		{
			//Cursor.SetClock(Inputs.Clock->AsShared());
		}

		~FAIGenOperator()
		{
			GenThread.Stop();
			//if (Generator != nullptr)
			//{
			//	Generator->Deinit();
			//}
		}

		class FCursor final : public FMidiPlayCursor
		{
		public:
			FCursor();

			virtual ~FCursor() override;

			void SetClock(const TSharedPtr<const HarmonixMetasound::FMidiClock, ESPMode::NotThreadSafe>& NewClock);

			void SetInterval(const Harmonix::FMusicTimeInterval& NewInterval);

			Harmonix::FMusicTimeInterval GetInterval() const { return Interval; }

			struct FPulseTime
			{
				int32 AudioBlockFrame;
				int32 MidiTick;
			};

			bool Pop(FPulseTime& PulseTime);

		private:
			void Push(FPulseTime&& PulseTime);

			virtual void AdvanceThruTick(int32 Tick, bool IsPreRoll) override;

			virtual void OnTimeSig(int32 TrackIndex, int32 Tick, int32 Numerator, int32 Denominator, bool IsPreroll) override;

			TWeakPtr<const HarmonixMetasound::FMidiClock, ESPMode::NotThreadSafe> Clock;
			Harmonix::FMusicTimeInterval Interval{};
			TSpscQueue<FPulseTime> PulsesSinceLastProcess;
			int32 QueueSize{ 0 };
			FTimeSignature CurrentTimeSignature{};
			FMusicTimestamp NextPulseTimestamp{ -1, -1 };
		};

		// Helper function for constructing vertex interface
		static const FVertexInterface& DeclareVertexInterface()
		{
			static const FVertexInterface Interface(
				FInputVertexInterface(
					TInputDataVertex<HarmonixMetasound::FMusicTransportEventStream>(METASOUND_GET_PARAM_NAME_AND_METADATA(AIGen::Inputs::Transport)),
					TInputDataVertex<HarmonixMetasound::FMidiClock>(METASOUND_GET_PARAM_NAME_AND_METADATA(AIGen::Inputs::Clock))
				),
				FOutputVertexInterface(
					TOutputDataVertex<HarmonixMetasound::FMidiStream>(METASOUND_GET_PARAM_NAME_AND_METADATA(AIGen::Outputs::MidiStream)),
					TOutputDataVertex<HarmonixMetasound::FMidiClock>(METASOUND_GET_PARAM_NAME_AND_METADATA(AIGen::Outputs::MidiClock))
				)
			);

			return Interface;
		}

		// Retrieves necessary metadata about your node
		static const FNodeClassMetadata& GetNodeInfo()
		{
			auto CreateNodeClassMetadata = []() -> FNodeClassMetadata
				{
					FVertexInterface NodeInterface = DeclareVertexInterface();

					FNodeClassMetadata Metadata
					{
						FNodeClassName { StandardNodes::Namespace, "AIGen Node", StandardNodes::AudioVariant },
						1, // Major Version
						1, // Minor Version
						METASOUND_LOCTEXT("AIGenNodeDisplayName", "AIGen Node"),
						METASOUND_LOCTEXT("AIGenNodeDesc", "A node that generates a MIDIStream."),
						PluginAuthor,
						PluginNodeMissingPrompt,
						NodeInterface,
						{ }, // Category Hierarchy 
						{ }, // Keywords for searching
						FNodeDisplayStyle{}
					};

					return Metadata;
				};

			static const FNodeClassMetadata Metadata = CreateNodeClassMetadata();
			return Metadata;
		}

		struct FInputs
		{
			HarmonixMetasound::FMidiClockReadRef Clock;
			HarmonixMetasound::FMusicTransportEventStreamReadRef Transport;
		};

		struct FOutputs
		{
			HarmonixMetasound::FMidiStreamWriteRef MidiStream;
			HarmonixMetasound::FMidiClockWriteRef  MidiClock;
		};


		virtual void BindInputs(FInputVertexInterfaceData& InOutVertexData) override
		{
			InOutVertexData.BindReadVertex("Clock", Inputs.Clock);
			InOutVertexData.BindReadVertex("Transport", Inputs.Transport);

			//SetClock(Inputs.Clock->AsShared());
			//Outputs.MidiClock->AttachToTimeAuthority(*Inputs.Clock);

			//Cursor.SetClock(Inputs.Clock->AsShared());
		}

		virtual void BindOutputs(FOutputVertexInterfaceData& InOutVertexData) override
		{
			InOutVertexData.BindReadVertex("MidiStream", Outputs.MidiStream);
			InOutVertexData.BindReadVertex("MidiClock", Outputs.MidiClock);
		}



		// Allows MetaSound graph to interact with your node's inputs
		virtual FDataReferenceCollection GetInputs() const override
		{
			FDataReferenceCollection InputDataReferences;

			InputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(AIGen::Inputs::Clock), Inputs.Clock);
			InputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(AIGen::Inputs::Transport), Inputs.Transport);

			return InputDataReferences;
		}

		// Allows MetaSound graph to interact with your node's outputs
		virtual FDataReferenceCollection GetOutputs() const override
		{
			FDataReferenceCollection OutputDataReferences;

			OutputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(AIGen::Outputs::MidiStream), Outputs.MidiStream);
			OutputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(AIGen::Outputs::MidiClock), Outputs.MidiClock);

			return OutputDataReferences;
		}

		static TUniquePtr<Metasound::IOperator> CreateOperator(const FBuildOperatorParams& InParams, FBuildResults& OutResults)
		{
			const FOperatorSettings& Settings = InParams.OperatorSettings;
			const FInputVertexInterfaceData& InputData = InParams.InputData;
			HarmonixMetasound::FMusicTransportEventStreamReadRef InTransport = InputData.GetOrConstructDataReadReference<HarmonixMetasound::FMusicTransportEventStream>(METASOUND_GET_PARAM_NAME(AIGen::Inputs::Transport), Settings);
			HarmonixMetasound::FMidiClockReadRef InMidiClock = InputData.GetOrConstructDataReadReference<HarmonixMetasound::FMidiClock>(METASOUND_GET_PARAM_NAME(AIGen::Inputs::Clock), Settings);
			return MakeUnique<FAIGenOperator>(InParams, InMidiClock, InTransport);
		}


	public:
		// From PulseGenerator.cpp
		FMidiVoiceGeneratorBase VoiceGenerator{};
		void Execute()
		{
			if (bShouldUpdateTokens)
			{
				TokenModifSection.Lock();
				UpdateScheduledMidiEvents();
				bShouldUpdateTokens = false;
				TokenModifSection.Unlock();
			}

			Outputs.MidiStream->PrepareBlock();

			Outputs.MidiClock->PrepareBlock();

			InitTransportIfNeeded();

			//if (Outputs.MidiClock->DoesLoop() != *LoopInPin || CurrentMidiFile != MidiAssetInPin->GetMidiProxy())
			//{
			//	SetupNewMidiFile(MidiAssetInPin->GetMidiProxy());
			//}

			//InitTransportIfNeeded();





			float speed = 1.0;

			Outputs.MidiClock->AddSpeedChangeToBlock({ 0, 0.0f, speed });

			TransportSpanPostProcessor MidiClockTransportHandler = [&](int32 StartFrameIndex, int32 EndFrameIndex, HarmonixMetasound::EMusicPlayerTransportState CurrentState)
			{
				int32 NumFrames = EndFrameIndex - StartFrameIndex;
				Outputs.MidiClock->HandleTransportChange(StartFrameIndex, CurrentState);
				Outputs.MidiClock->Process(StartFrameIndex, NumFrames, PrerollBars, speed);
			};

			TransportSpanProcessor TransportHandler = [this](int32 StartFrameIndex, int32 EndFrameIndex, HarmonixMetasound::EMusicPlayerTransportState CurrentState)
			{
				switch (CurrentState)
				{
				case HarmonixMetasound::EMusicPlayerTransportState::Starting:
					Outputs.MidiClock->ResetAndStart(StartFrameIndex, !ReceivedSeekWhileStopped());
					break;
				case HarmonixMetasound::EMusicPlayerTransportState::Seeking:
					Outputs.MidiClock->SeekTo(StartFrameIndex, Inputs.Transport->GetNextSeekDestination(), PrerollBars);
					break;
				}
				return GetNextTransportState(CurrentState);
			};
			ExecuteTransportSpans(Inputs.Transport, BlockSize, TransportHandler, MidiClockTransportHandler);
		}

		
		bool IsPlaying() const;

		////~ Begin FMidiPlayCursor interface
		//virtual void Reset(bool ForceNoBroadcast = false) override;
		//virtual void OnLoop(int32 LoopStartTick, int32 LoopEndTick) override;
		//virtual void SeekToTick(int32 Tick) override;
		//virtual void SeekThruTick(int32 Tick) override;
		//virtual void AdvanceThruTick(int32 Tick, bool IsPreRoll) override;

		//// Standard 1- or 2-byte MIDI message:
		virtual void OnMidiMessage(int32 TrackIndex, int32 Tick, uint8 Status, uint8 Data1, uint8 Data2, bool IsPreroll = false) override;
		//// Text, Copyright, TrackName, InstName, Lyric, Marker, CuePoint meta-event:
		////  "type" is the type of meta-event (constants defined in
		////  MidiFileConstants.h)
		//virtual void OnText(int32 TrackIndex, int32 Tick, int32 TextIndex, const FString& Str, uint8 Type, bool IsPreroll = false) override;
		//// Tempo Change meta-event:
		////  tempo is in microseconds per quarter-note
		//virtual void OnTempo(int32 TrackIndex, int32 Tick, int32 Tempo, bool IsPreroll = false) override;
		////// Time Signature meta-event:
		//////   time signature is numer/denom
		////virtual void OnTimeSig(int32 TrackIndex, int32 Tick, int32 Numerator, int32 Denominator, bool IsPreroll = false) override;
		////// Called when the look ahead amount changes or midi playback resets...
		////virtual void OnReset() override;
		//// Called when a noteOn happens during a pre-roll. Example usage...
		//// A cursor playing a synth that supports note-ons with start offsets into playback may
		//// pass this note on to the synth with the preRollMs as the start offset.
		//virtual void OnPreRollNoteOn(int32 TrackIndex, int32 EventTick, int32 Tick, float PreRollMs, uint8 Status, uint8 Data1, uint8 Data2) override;

		////~ End FMidiPlayCursor interface
		

		void Enable(bool bEnable)
		{
			Enabled = bEnable;
		}


		bool NeedsTransportInit = true;
		void InitTransportIfNeeded()
		{
			if (NeedsTransportInit)
			{
				InitTransportImpl();
				NeedsTransportInit = false;
			}
		}
		virtual void InitTransportImpl()
		{
			// Get the node caught up to its transport input
			FTransportInitFn InitFn = [this](HarmonixMetasound::EMusicPlayerTransportState CurrentState)
				{
					switch (CurrentState)
					{
					case HarmonixMetasound::EMusicPlayerTransportState::Invalid:
					case HarmonixMetasound::EMusicPlayerTransportState::Preparing:
					case HarmonixMetasound::EMusicPlayerTransportState::Prepared:
					case HarmonixMetasound::EMusicPlayerTransportState::Stopping:
					case HarmonixMetasound::EMusicPlayerTransportState::Killing:
						Outputs.MidiClock->AddTransportStateChangeToBlock({ 0, 0.0f, HarmonixMetasound::EMusicPlayerTransportState::Prepared });
						return HarmonixMetasound::EMusicPlayerTransportState::Prepared;

					case HarmonixMetasound::EMusicPlayerTransportState::Starting:
					case HarmonixMetasound::EMusicPlayerTransportState::Playing:
					case HarmonixMetasound::EMusicPlayerTransportState::Continuing:
						Outputs.MidiClock->ResetAndStart(0, !ReceivedSeekWhileStopped());
						return HarmonixMetasound::EMusicPlayerTransportState::Playing;

					case HarmonixMetasound::EMusicPlayerTransportState::Seeking: // seeking is omitted from init, shouldn't happen
						checkNoEntry();
						return HarmonixMetasound::EMusicPlayerTransportState::Invalid;

					case HarmonixMetasound::EMusicPlayerTransportState::Pausing:
					case HarmonixMetasound::EMusicPlayerTransportState::Paused:
						Outputs.MidiClock->AddTransportStateChangeToBlock({ 0, 0.0f, HarmonixMetasound::EMusicPlayerTransportState::Paused });
						return HarmonixMetasound::EMusicPlayerTransportState::Paused;

					default:
						checkNoEntry();
						return HarmonixMetasound::EMusicPlayerTransportState::Invalid;
					}
				};
			Init(*Inputs.Transport, MoveTemp(InitFn));
		}



	private:
		FInputs Inputs;
		FOutputs Outputs;

		FMidiFileProxyPtr MidiDataProxy;
		TSharedPtr<FMidiFileData> MidiFileData;

		bool bIsThreaded = true;
		TWeakObjectPtr<UMIDIGenerator> Generator = nullptr;
		TArray<int32> Tokens;
		FCursor Cursor;
		FSampleCount BlockSize = 0;
		int32 PrerollBars = 8;
		int32 UnplayedTokenIndex = 0;
		FGenThread GenThread;

		TOptional<HarmonixMetasound::FMidiStreamEvent> LastNoteOn;
		bool Enabled{ true };

		bool bShouldUpdateTokens = false;
		FCriticalSection TokenModifSection;
	};


	bool FAIGenOperator::IsPlaying() const
	{
		return GetTransportState() == HarmonixMetasound::EMusicPlayerTransportState::Playing
			|| GetTransportState() == HarmonixMetasound::EMusicPlayerTransportState::Starting
			|| GetTransportState() == HarmonixMetasound::EMusicPlayerTransportState::Continuing;
	}


	FAIGenOperator::FCursor::FCursor()
	{
		SetMessageFilter(EFilterPassFlags::TimeSig);
	}

	FAIGenOperator::FCursor::~FCursor()
	{
		// NB: If we let ~FMidiPlayCursor do this, we get warnings for potentially bad access
		SetClock(nullptr);
	}

	void FAIGenOperator::FCursor::SetClock(const TSharedPtr<const HarmonixMetasound::FMidiClock, ESPMode::NotThreadSafe>& NewClock)
	{
		if (Clock == NewClock)
		{
			return;
		}

		// unregister if we already have a clock
		if (const auto PinnedClock = Clock.Pin())
		{
			PinnedClock->UnregisterPlayCursor(this);
		}

		// register the new clock if we were given one
		if (NewClock.IsValid())
		{
			NewClock->RegisterHiResPlayCursor(this);
		}

		Clock = NewClock;
	}

	void FAIGenOperator::FCursor::SetInterval(const Harmonix::FMusicTimeInterval& NewInterval)
	{
		Interval = NewInterval;

		// Multiplier should be >= 1
		Interval.IntervalMultiplier = FMath::Max(Interval.IntervalMultiplier, static_cast<uint16>(1));
	}

	bool FAIGenOperator::FCursor::Pop(FPulseTime& PulseTime)
	{
		if (PulsesSinceLastProcess.Dequeue(PulseTime))
		{
			--QueueSize;
			return true;
		}

		return false;
	}

	void FAIGenOperator::FCursor::Push(FPulseTime&& PulseTime)
	{
		constexpr int32 MaxQueueSize = 1024; // just to keep the queue from infinitely growing if we stop draining it
		if (QueueSize < MaxQueueSize)
		{
			PulsesSinceLastProcess.Enqueue(MoveTemp(PulseTime));
			++QueueSize;
		}
	}

	void FAIGenOperator::FCursor::AdvanceThruTick(int32 Tick, bool IsPreRoll)
	{
		FMidiPlayCursor::AdvanceThruTick(Tick, IsPreRoll);

		if (!NextPulseTimestamp.IsValid())
		{
			return;
		}

		check(Clock.IsValid()); // if we're here and we don't have a clock, we have problems
		const auto PinnedClock = Clock.Pin();
		int32 NextPulseTick = PinnedClock->GetBarMap().MusicTimestampToTick(NextPulseTimestamp);

		while (CurrentTick >= NextPulseTick)
		{
			Push({ PinnedClock->GetCurrentBlockFrameIndex(), NextPulseTick });

			IncrementTimestampByInterval(NextPulseTimestamp, Interval, CurrentTimeSignature);

			NextPulseTick = PinnedClock->GetBarMap().MusicTimestampToTick(NextPulseTimestamp);
		}
	}

	void FAIGenOperator::FCursor::OnTimeSig(int32 TrackIndex, int32 Tick, int32 Numerator, int32 Denominator, bool IsPreroll)
	{
		CurrentTimeSignature.Numerator = Numerator;
		CurrentTimeSignature.Denominator = Denominator;

		check(Clock.IsValid()); // if we're here and we don't have a clock, we have problems
		const auto PinnedClock = Clock.Pin();
		// Time sig changes will come on the downbeat, and if we change time signature,
		// we want to reset the pulse, so the next pulse is now plus the offset
		NextPulseTimestamp = PinnedClock->GetBarMap().TickToMusicTimestamp(Tick);
		IncrementTimestampByOffset(NextPulseTimestamp, Interval, CurrentTimeSignature);
	}

	// Standard 1- or 2-byte MIDI message:
	void FAIGenOperator::OnMidiMessage(int32 TrackIndex, int32 Tick, uint8 Status, uint8 Data1, uint8 Data2, bool IsPreroll)
	{
		if (IsPlaying())
		{
			HarmonixMetasound::FMidiStreamEvent MidiEvent(this, FMidiMsg(Status, Data1, Data2));
			MidiEvent.BlockSampleFrameIndex = Outputs.MidiClock->GetCurrentBlockFrameIndex();
			MidiEvent.AuthoredMidiTick = Tick;
			MidiEvent.CurrentMidiTick = Tick;
			MidiEvent.TrackIndex = TrackIndex;
			Outputs.MidiStream->AddMidiEvent(MidiEvent);
		}
	}
	//// Text, Copyright, TrackName, InstName, Lyric, Marker, CuePoint meta-event:
	////  "type" is the type of meta-event (constants defined in
	////  MidiFileConstants.h)
	//void FAIGenOperator::OnText(int32 TrackIndex, int32 Tick, int32 TextIndex, const FString& Str, uint8 Type, bool IsPreroll)
	//{
	//	if (!IsPreroll && IsPlaying())
	//	{
	//		HarmonixMetasound::FMidiStreamEvent MidiEvent(this, FMidiMsg::CreateText(TextIndex, Type));
	//		MidiEvent.BlockSampleFrameIndex = Outputs.MidiClock->GetCurrentBlockFrameIndex();
	//		MidiEvent.AuthoredMidiTick = Tick;
	//		MidiEvent.CurrentMidiTick = Tick;
	//		MidiEvent.TrackIndex = TrackIndex;
	//		Outputs.MidiStream->AddMidiEvent(MidiEvent);
	//	}
	//}
	//// Tempo Change meta-event:
	////  tempo is in microseconds per quarter-note
	//void FAIGenOperator::OnTempo(int32 TrackIndex, int32 Tick, int32 Tempo, bool IsPreroll)
	//{
	//	if (IsPlaying())
	//	{
	//		HarmonixMetasound::FMidiStreamEvent MidiEvent(this, FMidiMsg((int32)Tempo));
	//		MidiEvent.BlockSampleFrameIndex = Outputs.MidiClock->GetCurrentBlockFrameIndex();
	//		MidiEvent.AuthoredMidiTick = Tick;
	//		MidiEvent.CurrentMidiTick = Tick;
	//		MidiEvent.TrackIndex = TrackIndex;
	//		Outputs.MidiStream->AddMidiEvent(MidiEvent);
	//	}
	//}
	////// Time Signature meta-event:
	//////   time signature is numer/denom
	////void FAIGenOperator::OnTimeSig(int32 TrackIndex, int32 Tick, int32 Numerator, int32 Denominator, bool IsPreroll)
	////{

	////}
	////// Called when the look ahead amount changes or midi playback resets...
	////void FAIGenOperator::OnReset()
	////{

	//// Called when a noteOn happens during a pre-roll. Example usage...
	//// A cursor playing a synth that supports note-ons with start offsets into playback may
	//// pass this note on to the synth with the preRollMs as the start offset.
	//void FAIGenOperator::OnPreRollNoteOn(int32 TrackIndex, int32 EventTick, int32 Tick, float PreRollMs, uint8 Status, uint8 Data1, uint8 Data2)
	//{
	//	if (IsPlaying())
	//	{
	//		HarmonixMetasound::FMidiStreamEvent MidiEvent(this, FMidiMsg(Status, Data1, Data2));
	//		MidiEvent.BlockSampleFrameIndex = Outputs.MidiClock->GetCurrentBlockFrameIndex();
	//		MidiEvent.AuthoredMidiTick = EventTick;
	//		MidiEvent.CurrentMidiTick = Tick;
	//		MidiEvent.TrackIndex = TrackIndex;
	//		Outputs.MidiStream->AddMidiEvent(MidiEvent);
	//	}
	//}

	////void FAIGenOperator::Reset(const FResetParams& Params)
	////{
	////	BlockSize = Params.OperatorSettings.GetNumFramesPerBlock();
	////	//CurrentBlockSpanStart = 0;

	////	Outputs.MidiStream->SetClock(*Outputs.MidiClock);
	////	Outputs.MidiClock->ResetAndStart(0, true);

	////	//NeedsTransportInit = true;
	////}

	//void FAIGenOperator::Reset(bool ForceNoBroadcast /*= false*/)
	//{
	//	FMidiPlayCursor::Reset(ForceNoBroadcast);
	//}

	//void FAIGenOperator::OnLoop(int32 LoopStartTick, int32 LoopEndTick)
	//{
	//	//TRACE_BOOKMARK(TEXT("Midi Looping"));
	//	FMidiPlayCursor::OnLoop(LoopStartTick, LoopEndTick);
	//}

	//void FAIGenOperator::SeekToTick(int32 Tick)
	//{
	//	//TRACE_BOOKMARK(TEXT("Midi Seek To Tick"));
	//	FMidiPlayCursor::SeekToTick(Tick);

	//	//if (IsPlaying())
	//	//{
	//	//	FMidiStreamEvent MidiEvent(this, bKillVoicesOnSeek ? FMidiMsg::CreateAllNotesKill() : FMidiMsg::CreateAllNotesOff());
	//	//	MidiEvent.BlockSampleFrameIndex = MidiClockOut->GetCurrentBlockFrameIndex();
	//	//	MidiEvent.AuthoredMidiTick = MidiClockOut->GetCurrentHiResTick();
	//	//	MidiEvent.CurrentMidiTick = MidiClockOut->GetCurrentHiResTick();
	//	//	MidiEvent.TrackIndex = 0;
	//	//	MidiOutPin->AddMidiEvent(MidiEvent);
	//	//}
	//}

	//void FAIGenOperator::SeekThruTick(int32 Tick)
	//{
	//	//TRACE_BOOKMARK(TEXT("Midi Seek Thru Tick"));
	//	FMidiPlayCursor::SeekThruTick(Tick);

	//	//if (IsPlaying())
	//	//{
	//	//	FMidiStreamEvent MidiEvent(this, bKillVoicesOnSeek ? FMidiMsg::CreateAllNotesKill() : FMidiMsg::CreateAllNotesOff());
	//	//	MidiEvent.BlockSampleFrameIndex = MidiClockOut->GetCurrentBlockFrameIndex();
	//	//	MidiEvent.AuthoredMidiTick = MidiClockOut->GetCurrentHiResTick();
	//	//	MidiEvent.CurrentMidiTick = MidiClockOut->GetCurrentHiResTick();
	//	//	MidiEvent.TrackIndex = 0;
	//	//	MidiOutPin->AddMidiEvent(MidiEvent);
	//	//}
	//}

	//void FAIGenOperator::AdvanceThruTick(int32 Tick, bool IsPreRoll)
	//{
	//	FMidiPlayCursor::AdvanceThruTick(Tick, IsPreRoll);
	//}



	// Node Class - Inheriting from FNodeFacade is recommended for nodes that have a static FVertexInterface
	class FAIGenNode : public FNodeFacade
	{
	public:
		FAIGenNode(const FNodeInitData& InitData)
			: FNodeFacade(InitData.InstanceName, InitData.InstanceID, TFacadeOperatorClass<FAIGenOperator>())
		{
		}
	};






	// Register node
	METASOUND_REGISTER_NODE(FAIGenNode);
}
















#undef LOCTEXT_NAMESPACE