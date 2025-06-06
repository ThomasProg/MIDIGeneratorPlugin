// Copyright Prog'z. All Rights Reserved.

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
#include "MetasoundMIDIGenerator.h"
#include "GenThread.h"
#include "MIDIGeneratorEnv.h"

#define IS_VERSION(MAJOR, MINOR) (ENGINE_MAJOR_VERSION == MAJOR) && (ENGINE_MINOR_VERSION == MINOR)
#define IS_VERSION_OR_PREV(MAJOR, MINOR) (ENGINE_MAJOR_VERSION == MAJOR) && (ENGINE_MINOR_VERSION <= MINOR)
#define IS_VERSION_OR_AFTER(MAJOR, MINOR) (ENGINE_MAJOR_VERSION == MAJOR) && (ENGINE_MINOR_VERSION >= MINOR)

#if IS_VERSION_OR_AFTER(5, 6)
#include "HarmonixMidi/MIDICursor.h"
#endif

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
#if IS_VERSION_OR_PREV(5, 4)
	class FAIGenOperator : public TExecutableOperator<FAIGenOperator>, public FMidiVoiceGeneratorBase, public HarmonixMetasound::FMusicTransportControllable, public FMidiPlayCursor
#elif IS_VERSION_OR_AFTER(5, 6)
	class FAIGenOperator : public TExecutableOperator<FAIGenOperator>, public FMidiVoiceGeneratorBase, public HarmonixMetasound::FMusicTransportControllable, public FMidiCursor::FReceiver
#endif
	{
	public:
		FAIGenOperator(const Metasound::FBuildOperatorParams& InParams,
			const HarmonixMetasound::FMidiClockReadRef& InMidiClock,
			const HarmonixMetasound::FMusicTransportEventStreamReadRef& InTransport,
			const Metasound::FMIDIGeneratorZZZReadRef& InGenerator)
			: FMusicTransportControllable(HarmonixMetasound::EMusicPlayerTransportState::Prepared)
			, Inputs{ InMidiClock, InTransport, InGenerator }
			, Outputs{ HarmonixMetasound::FMidiStreamWriteRef::CreateNew(), HarmonixMetasound::FMidiClockWriteRef::CreateNew(InParams.OperatorSettings) }
			, BlockSize(InParams.OperatorSettings.GetNumFramesPerBlock())
		{
			TryUpdateGenThreadInput();
		}

		void OnGenEnvSet(FMIDIGeneratorEnv& gen)
		{
			Outputs.MidiStream->SetMidiFile(gen.MidiDataProxy);
			gen.SetClock(*Outputs.MidiClock);

#if IS_VERSION_OR_PREV(5, 4)
			Outputs.MidiClock->RegisterHiResPlayCursor(this);
			Outputs.MidiStream->SetClock(*Outputs.MidiClock);
			Outputs.MidiClock->AttachToMidiResource(gen.MidiFileData);
#elif IS_VERSION_OR_AFTER(5, 6)
			Outputs.MidiClock->AttachToSongMapEvaluator(gen.MidiDataProxy->GetMidiFile(), !IsPlaying());
			MidiCursor.Prepare(gen.MidiDataProxy->GetMidiFile());
			Outputs.MidiStream->SetTicksPerQuarterNote(gen.MidiDataProxy->GetMidiFile()->TicksPerQuarterNote);
			Outputs.MidiClock->ClearPersistentLoop();
			MidiCursor.SeekToNextTick(Outputs.MidiClock->GetNextMidiTickToProcess(), PrerollBars, this);
#endif
		}

		void TryUpdateGenThreadInput()
		{
			TSharedPtr<FMIDIGeneratorEnv> NewGen;
			if (Inputs.Generator.Get() != nullptr && Inputs.Generator->GetProxy().IsValid())
			{
				NewGen = Inputs.Generator->GetProxy()->MidiGenerator;
			}

			if (Generator != NewGen)
			{
				Generator = NewGen;

				if (Generator != nullptr)
				{
					OnGenEnvSet(*Generator);

					if (!Generator->GenThread->HasStarted())
					{
						Generator->StartGeneration();
					}

					Generator->DecodeTokens();
				}
			}
		}

		MidiTokenizerHandle GetTok() const
		{
			return Generator->GenThread->GetTok().GetTokenizer();
		}

		MusicGeneratorHandle GetGen() const
		{
			return Generator->GenThread->GetGen();
		}

		~FAIGenOperator()
		{
			//destroyMidiConverter(converter);
			if (Generator.IsValid())
			{
				//OnRemoveGen();
				Generator->StopGeneration();
			}
		}

		// Helper function for constructing vertex interface
		static const FVertexInterface& DeclareVertexInterface()
		{
			static const FVertexInterface Interface(
				FInputVertexInterface(
					TInputDataVertex<HarmonixMetasound::FMusicTransportEventStream>(METASOUND_GET_PARAM_NAME_AND_METADATA(AIGen::Inputs::Transport)),
					TInputDataVertex<HarmonixMetasound::FMidiClock>(METASOUND_GET_PARAM_NAME_AND_METADATA(AIGen::Inputs::Clock)),
					TInputDataVertex<Metasound::FMIDIGeneratorZZZ>("Generator", FDataVertexMetadata{ FText::FromString("Generates"), FText::FromString("Generator") })
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
			Metasound::FMIDIGeneratorZZZReadRef Generator;
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
			InOutVertexData.BindReadVertex("Generator", Inputs.Generator);
		}

		virtual void BindOutputs(FOutputVertexInterfaceData& InOutVertexData) override
		{
			InOutVertexData.BindWriteVertex("MidiStream", Outputs.MidiStream);
			InOutVertexData.BindWriteVertex("MidiClock", Outputs.MidiClock);
		}

		static TUniquePtr<Metasound::IOperator> CreateOperator(const FBuildOperatorParams& InParams, FBuildResults& OutResults)
		{
			const FOperatorSettings& Settings = InParams.OperatorSettings;
			const FInputVertexInterfaceData& InputData = InParams.InputData;
			HarmonixMetasound::FMusicTransportEventStreamReadRef InTransport = InputData.GetOrConstructDataReadReference<HarmonixMetasound::FMusicTransportEventStream>(METASOUND_GET_PARAM_NAME(AIGen::Inputs::Transport), Settings);
			HarmonixMetasound::FMidiClockReadRef InMidiClock = InputData.GetOrConstructDataReadReference<HarmonixMetasound::FMidiClock>(METASOUND_GET_PARAM_NAME(AIGen::Inputs::Clock), Settings);
			Metasound::FMIDIGeneratorZZZReadRef InGenerator = InputData.GetOrConstructDataReadReference<Metasound::FMIDIGeneratorZZZ>("Generator");
			return MakeUnique<FAIGenOperator>(InParams, InMidiClock, InTransport, InGenerator);
		}


	public:
		void Execute()
		{
			SCOPE_CYCLE_COUNTER(STAT_MidiGen);

			TryUpdateGenThreadInput();

			if ((!Generator.IsValid()) || GetTok() == nullptr)
			{
				return;
			}

			Generator->ClockLock.Lock();
#if IS_VERSION_OR_PREV(5, 4)
			Generator->CurrentTick = Outputs.MidiClock->GetCurrentHiResTick();
#elif IS_VERSION_OR_AFTER(5, 6)
			Generator->CurrentTick = Outputs.MidiClock->GetNextMidiTickToProcess();
#endif
#if IS_VERSION_OR_PREV(5, 4)
			Outputs.MidiClock->GetDrivingMidiPlayCursorMgr()->LockForMidiDataChanges();
#endif
			Generator->DecodeTokens();
#if IS_VERSION_OR_PREV(5, 4)
			Outputs.MidiClock->GetDrivingMidiPlayCursorMgr()->MidiDataChangeComplete(FMidiPlayCursorMgr::EMidiChangePositionCorrectMode::MaintainTick);
#endif
			Generator->ClockLock.Unlock();

			Outputs.MidiStream->PrepareBlock();
			CurrentRenderBlockFrame = 0;

			Outputs.MidiClock->PrepareBlock();

			InitTransportIfNeeded();

			float speed = 1.0;
#if IS_VERSION_OR_PREV(5, 4)
			Outputs.MidiClock->AddSpeedChangeToBlock({ 0, 0.0f, speed });
#elif IS_VERSION_OR_AFTER(5, 6) 
			Outputs.MidiClock->SetSpeed(0, speed);
#endif

			TransportSpanPostProcessor MidiClockTransportHandler = [&](int32 StartFrameIndex, int32 EndFrameIndex, HarmonixMetasound::EMusicPlayerTransportState CurrentState)
			{
				int32 NumFrames = EndFrameIndex - StartFrameIndex;
#if IS_VERSION_OR_PREV(5, 4)
				Outputs.MidiClock->HandleTransportChange(StartFrameIndex, CurrentState);
				Outputs.MidiClock->Process(StartFrameIndex, NumFrames, PrerollBars, speed);
#elif IS_VERSION_OR_AFTER(5, 6) 
				Outputs.MidiClock->SetTransportState(StartFrameIndex, CurrentState);
				switch (CurrentState)
				{
				case HarmonixMetasound::EMusicPlayerTransportState::Playing:
				case HarmonixMetasound::EMusicPlayerTransportState::Continuing:
					Outputs.MidiClock->Advance(StartFrameIndex, NumFrames);
				}
#endif
			};

			TransportSpanProcessor TransportHandler = [this](int32 StartFrameIndex, int32 EndFrameIndex, HarmonixMetasound::EMusicPlayerTransportState CurrentState)
			{
				switch (CurrentState)
				{
				case HarmonixMetasound::EMusicPlayerTransportState::Starting:
#if IS_VERSION_OR_PREV(5, 4)
					Outputs.MidiClock->ResetAndStart(StartFrameIndex, !ReceivedSeekWhileStopped());
#elif IS_VERSION_OR_AFTER(5, 6) 
					if (!ReceivedSeekWhileStopped())
					{
						Outputs.MidiClock->SeekTo(StartFrameIndex, 0, 0);
					}
#endif
					break;
				case HarmonixMetasound::EMusicPlayerTransportState::Seeking:
#if IS_VERSION_OR_PREV(5, 4)
					Outputs.MidiClock->SeekTo(StartFrameIndex, Inputs.Transport->GetNextSeekDestination(), PrerollBars);
#elif IS_VERSION_OR_AFTER(5, 6) 
					Outputs.MidiClock->SeekTo(StartFrameIndex, Inputs.Transport->GetNextSeekDestination());
#endif
					break;
				}
				return GetNextTransportState(CurrentState);
			};
			ExecuteTransportSpans(Inputs.Transport, BlockSize, TransportHandler, MidiClockTransportHandler);
#if IS_VERSION_OR_AFTER(5, 6)
			RenderMidiForClockEvents();
#endif
		}

#if IS_VERSION_OR_AFTER(5, 6)
		void RenderMidiForClockEvents()
		{
			const TArray<HarmonixMetasound::FMidiClockEvent>& ClockEvents = Outputs.MidiClock->GetMidiClockEventsInBlock();
			for (const HarmonixMetasound::FMidiClockEvent& Event : ClockEvents)
			{
				CurrentRenderBlockFrame = Event.BlockFrameIndex;
				if (auto AsAdvance = Event.TryGet<HarmonixMetasound::MidiClockMessageTypes::FAdvance>())
				{
					MidiCursor.Process(AsAdvance->FirstTickToProcess, AsAdvance->LastTickToProcess(), *this);
				}
				// There's no looping or seeking
				
				//else if (auto AsSeekTo = Event.TryGet<HarmonixMetasound::MidiClockMessageTypes::FSeek>())
				//{
				//	SendAllNotesOff(Event.BlockFrameIndex, AsSeekTo->NewNextTick);
				//	MidiCursor.SeekToNextTick(AsSeekTo->NewNextTick, PrerollBars, this);
				//}
				//else if (auto AsLoop = Event.TryGet<HarmonixMetasound::MidiClockMessageTypes::FLoop>())
				//{
				//	// When looping we don't preroll the events prior to the 
				//	// loop start point.
				//	SendAllNotesOff(Event.BlockFrameIndex, AsLoop->FirstTickInLoop);
				//	MidiCursor.SeekToNextTick(AsLoop->FirstTickInLoop);
				//}
			}
		}
#endif
		
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

#if IS_VERSION_OR_AFTER(5, 6)
		int32 CurrentRenderBlockFrame = 0;
		FMidiCursor MidiCursor;
#endif

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
#if IS_VERSION_OR_PREV(5, 4)
						//Outputs.MidiClock->AddTransportStateChangeToBlock({ 0, 0.0f, HarmonixMetasound::EMusicPlayerTransportState::Prepared });
#elif IS_VERSION_OR_AFTER(5, 6)
						Outputs.MidiClock->SetTransportState(0, HarmonixMetasound::EMusicPlayerTransportState::Prepared);
#endif
						return HarmonixMetasound::EMusicPlayerTransportState::Prepared;

					case HarmonixMetasound::EMusicPlayerTransportState::Starting:
					case HarmonixMetasound::EMusicPlayerTransportState::Playing:
					case HarmonixMetasound::EMusicPlayerTransportState::Continuing:
#if IS_VERSION_OR_PREV(5, 4)
						Outputs.MidiClock->ResetAndStart(0, !ReceivedSeekWhileStopped());
#elif IS_VERSION_OR_AFTER(5, 6)
						Outputs.MidiClock->SetTransportState(0, HarmonixMetasound::EMusicPlayerTransportState::Playing);
						if (!ReceivedSeekWhileStopped())
						{
							Outputs.MidiClock->SeekTo(0, 0, 0);
						}
#endif
						return HarmonixMetasound::EMusicPlayerTransportState::Playing;

					case HarmonixMetasound::EMusicPlayerTransportState::Seeking: // seeking is omitted from init, shouldn't happen
						checkNoEntry();
						return HarmonixMetasound::EMusicPlayerTransportState::Invalid;

					case HarmonixMetasound::EMusicPlayerTransportState::Pausing:
					case HarmonixMetasound::EMusicPlayerTransportState::Paused:
#if IS_VERSION_OR_PREV(5, 4)
						Outputs.MidiClock->AddTransportStateChangeToBlock({ 0, 0.0f, HarmonixMetasound::EMusicPlayerTransportState::Paused });
#elif IS_VERSION_OR_AFTER(5, 6)
						Outputs.MidiClock->SetTransportState(0, HarmonixMetasound::EMusicPlayerTransportState::Paused);
#endif
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

		FSampleCount BlockSize = 0;
		int32 PrerollBars = 8;
		TSharedPtr<FMIDIGeneratorEnv> Generator;

		bool Enabled{ true };
	};


	bool FAIGenOperator::IsPlaying() const
	{
		return GetTransportState() == HarmonixMetasound::EMusicPlayerTransportState::Playing
			|| GetTransportState() == HarmonixMetasound::EMusicPlayerTransportState::Starting
			|| GetTransportState() == HarmonixMetasound::EMusicPlayerTransportState::Continuing;
	}

	// Standard 1- or 2-byte MIDI message:
	void FAIGenOperator::OnMidiMessage(int32 TrackIndex, int32 Tick, uint8 Status, uint8 Data1, uint8 Data2, bool IsPreroll)
	{
		if (IsPlaying())
		{
			HarmonixMetasound::FMidiStreamEvent MidiEvent(this, FMidiMsg(Status, Data1, Data2));
#if IS_VERSION_OR_PREV(5, 4)
			MidiEvent.BlockSampleFrameIndex = Outputs.MidiClock->GetCurrentBlockFrameIndex();
#elif IS_VERSION_OR_AFTER(5, 6)
			MidiEvent.BlockSampleFrameIndex = CurrentRenderBlockFrame;
#endif
			MidiEvent.AuthoredMidiTick = Tick;
			MidiEvent.CurrentMidiTick = Tick;
			MidiEvent.TrackIndex = TrackIndex;
			Outputs.MidiStream->AddMidiEvent(MidiEvent);

			//if (Status == 144 && Data1 == 70)
			//{
			//	UE_LOG(LogTemp, Warning, TEXT("=== Note played at tick %d / Clock current: %d ==="), Tick, Outputs.MidiClock->GetCurrentHiResTick());
			//}
		}
	}

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