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
			

			if (Generator == nullptr)
			{
				Generator = NewObject<UMIDIGenerator>();
			}

			Generator->Init();
			Generator->Generate(120, Tokens);

			UpdateScheduledMidiEvents();

			Reset(InParams);

			Outputs.MidiClock->AttachToMidiResource(MidiFileData);
		}

		TArray<HarmonixMetasound::FMidiStreamEvent> EventsToPlay;

		void UpdateScheduledMidiEvents()
		{
			std::int32_t i = UnplayedTokenIndex;

			struct Args
			{
				std::int32_t& i;
				std::int32_t& unplayedTokenIndex;
				FAIGenOperator* self;

				int32 LastTick = 0;
			};

			Args args{ i, UnplayedTokenIndex, this};

			MidiConverterHandle converter = createMidiConverter();
			converterSetTokenizer(converter, Generator->tok);

			//Outputs.MidiClock->LockForMidiDataChanges();

			MidiFileData->Tracks[0].Empty();
			converterSetOnNote(converter, [](void* data, const Note& newNote)
				{
					Args& args = *(Args*)(data);
					args.unplayedTokenIndex = args.i + 1;

					//args.self->Cursor.p

					int32 Channel = 1;
					int32 NoteNumber = newNote.pitch;
					int32 Velocity = newNote.velocity;

					int32 Track = 0;
					int32 Tick = newNote.tick*100;

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

			while (i < Tokens.Num())
			{
				converterProcessToken(converter, Tokens.GetData(), Tokens.Num(), i, &args);
				i++;
			}

			//Outputs.MidiClock->MidiDataChangesComplete(FMidiPlayCursorMgr::EMidiChangePositionCorrectMode::MaintainTick);

			destroyMidiConverter(converter);
		}

		void Reset(const FResetParams&)
		{
			//Cursor.SetClock(Inputs.Clock->AsShared());
			ApplyParameters();
		}

		~FAIGenOperator()
		{
			if (Generator != nullptr)
			{
				Generator->Deinit();
			}
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
			ApplyParameters();

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

		void ApplyParameters()
		{
			//PulseGenerator.Track = *Inputs.Track;
			//PulseGenerator.Channel = *Inputs.Channel;
			//PulseGenerator.NoteNumber = *Inputs.NoteNumber;
			//PulseGenerator.Velocity = *Inputs.Velocity;
			//PulseGenerator.SetInterval(
			//	{
			//		*Inputs.Interval,
			//		*Inputs.Offset,
			//		static_cast<uint16>(*Inputs.IntervalMultiplier),
			//		static_cast<uint16>(*Inputs.OffsetMultiplier)
			//	});
		}


	public:
		int32 t = 0;

		// From PulseGenerator.cpp
		FMidiVoiceGeneratorBase VoiceGenerator{};
		void Execute()
		{
			ApplyParameters();

			//Outputs.MidiStream->PrepareBlock();
			//Outputs.MidiClock->PrepareBlock();

			//int32 Channel = 1;
			//int32 NoteNumber = 60;
			//int32 Velocity = 120;

			//int32 Track = 0;
			//int32 Tick = t;


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












			////t += 5;
			//if (t == 0)
			//{
			//	t = 1;

			//	int32 NoteOnSample = 0;
			//	FMidiMsg Msg{ FMidiMsg::CreateNoteOn(Channel - 1, NoteNumber, Velocity) };
			//	HarmonixMetasound::FMidiStreamEvent Event{ &VoiceGenerator, Msg };
			//	Event.BlockSampleFrameIndex = NoteOnSample;
			//	Event.AuthoredMidiTick = Tick;
			//	Event.CurrentMidiTick = Tick;
			//	Event.TrackIndex = Track;
			//	Outputs.MidiStream->InsertMidiEvent(Event);
			//}





			//// Keep draining the queue if disabled, so we get the next note off,
			//// and so we stay in phase if we toggle off and back on
			//FCursor::FPulseTime NextPulse;
			//while (Cursor.Pop(NextPulse))
			//{
			//	int32 NoteOnSample = NextPulse.AudioBlockFrame;

			//	// Note off if there was a previous note on
			//	if (LastNoteOn.IsSet())
			//	{
			//		check(LastNoteOn->MidiMessage.IsNoteOn());

			//		// Trigger the note off one sample before the note on
			//		const int32 NoteOffSample = NextPulse.AudioBlockFrame > 0 ? NextPulse.AudioBlockFrame - 1 : NextPulse.AudioBlockFrame;
			//		NoteOnSample = NoteOffSample + 1;

			//		// Trigger the note off one tick before the note on
			//		const int32 NoteOffTick = NextPulse.MidiTick - 1;

			//		FMidiMsg Msg{ FMidiMsg::CreateNoteOff(LastNoteOn->MidiMessage.GetStdChannel(), LastNoteOn->MidiMessage.GetStdData1()) };
			//		HarmonixMetasound::FMidiStreamEvent Event{ &VoiceGenerator, Msg };
			//		Event.BlockSampleFrameIndex = NoteOffSample;
			//		Event.AuthoredMidiTick = NoteOffTick;
			//		Event.CurrentMidiTick = NoteOffTick;
			//		Event.TrackIndex = LastNoteOn->TrackIndex;
			//		Outputs.MidiStream->InsertMidiEvent(Event);

			//		LastNoteOn.Reset();
			//	}

			//	// Note on
			//	if (Enabled && !EventsToPlay.IsEmpty())
			//	{
			//		HarmonixMetasound::FMidiStreamEvent& FrontEvent = EventsToPlay[0];
			//		if (FrontEvent.CurrentMidiTick < NextPulse.MidiTick)
			//		{
			//			HarmonixMetasound::FMidiStreamEvent Event = EventsToPlay[0];
			//			EventsToPlay.RemoveAt(0);

			//			// Note ons must pass note number and velocity check
			//			if (Event.MidiMessage.IsNoteOn())
			//				{
			//				const uint8 NoteNumber2 = Event.MidiMessage.GetStdData1();
			//				const uint8 Velocity2 = Event.MidiMessage.GetStdData2();
			//				const uint8 br = 0;
			//			}

			//			//FMidiMsg Msg{ FMidiMsg::CreateNoteOn(Channel - 1, NoteNumber, Velocity) };
			//			//HarmonixMetasound::FMidiStreamEvent Event{ &VoiceGenerator, Msg };
			//			//Event.BlockSampleFrameIndex = NoteOnSample;
			//			//Event.AuthoredMidiTick = NextPulse.MidiTick;
			//			//Event.CurrentMidiTick = NextPulse.MidiTick;
			//			//Event.TrackIndex = Track;
			//			Event.BlockSampleFrameIndex = NoteOnSample;
			//			Outputs.MidiStream->InsertMidiEvent(Event);

			//			LastNoteOn.Emplace(MoveTemp(Event));
			//		}
			//	}
			//}
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
		TWeakObjectPtr<UMIDIGenerator> Generator = nullptr;
		TArray<int32> Tokens;
		FCursor Cursor;
		FSampleCount BlockSize = 0;
		int32 PrerollBars = 8;
		int32 UnplayedTokenIndex = 0;

		TOptional<HarmonixMetasound::FMidiStreamEvent> LastNoteOn;
		bool Enabled{ true };
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