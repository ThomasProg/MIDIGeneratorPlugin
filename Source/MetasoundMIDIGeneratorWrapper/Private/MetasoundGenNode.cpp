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
			//int32 CurrentTempo = 120;
			//int32 CurrentTimeSigNum = 4;
			//int32 CurrentTimeSigDenom = 4;
			//gen.MidiFileData = HarmonixMetasound::FMidiClock::MakeClockConductorMidiData(CurrentTempo, CurrentTimeSigNum, CurrentTimeSigDenom);

			//gen.MidiFileData->MidiFileName = "Generated";
			//gen.MidiFileData->TicksPerQuarterNote = 16;
			//FMidiTrack track;
			//track.AddEvent(FMidiEvent(0, FMidiMsg::CreateAllNotesKill()));
			//gen.MidiFileData->Tracks.Add(MoveTemp(track));

			//gen.MidiDataProxy = MakeShared<FMidiFileProxy, ESPMode::ThreadSafe>(gen.MidiFileData);


			Outputs.MidiStream->SetMidiFile(gen.MidiDataProxy);

			Outputs.MidiClock->RegisterHiResPlayCursor(this);
			Outputs.MidiStream->SetClock(*Outputs.MidiClock);
			//Outputs.MidiStream->SetTicksPerQuarterNote(1);

			Outputs.MidiClock->AttachToMidiResource(gen.MidiFileData);
		}

		std::int32_t nextTokenToProcess = 0;
		MidiConverterHandle converter = nullptr;
		int32 AddedTicks = 0;

		void OnRemoveGen()
		{
			if (Generator != nullptr)
			{
				TokenModifSection.Lock();
				Generator->GenThread->SetOnGenerated(nullptr);
				Generator->NewEncodedTokens.Empty();
				TokenModifSection.Unlock();
			}
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
				OnRemoveGen();
				//if (Generator != nullptr)
				//{
				//	TokenModifSection.Lock();
				//	Generator->GenThread->OnGenerated.Unbind();
				//	NewEncodedTokens.Empty();
				//	TokenModifSection.Unlock();
				//}

				Generator = NewGen;

				if (Generator != nullptr)
				{
					TokenModifSection.Lock();
					OnGenEnvSet(*Generator);
					Generator->GenThread->SetOnGenerated(FOnGenerated::CreateLambda([this](int32 NewToken)
						{
							TokenModifSection.Lock();
							bShouldUpdateTokens = true;
							Generator->NewEncodedTokens.Add(NewToken);
							TokenModifSection.Unlock();
						}));
					TokenModifSection.Unlock();

					if (!Generator->GenThread->HasStarted())
					{
						Generator->StartGeneration();
					}

					TokenModifSection.Lock();
					Generator->GenThread->GetEncodedTokens(Generator->NewEncodedTokens);
					UpdateScheduledMidiEvents();
					TokenModifSection.Unlock();
				}
			}
		}

		MidiTokenizerHandle GetTok() const
		{
			return Generator->GenThread->GetTok();
		}

		MusicGeneratorHandle GetGen() const
		{
			return Generator->GenThread->GetGen();
		}

		void UpdateScheduledMidiEvents()
		{
			if (GetTok() == nullptr)
			{
				return;
			}

			Outputs.MidiClock->GetDrivingMidiPlayCursorMgr()->LockForMidiDataChanges();

			struct Args
			{
				FAIGenOperator* self;

				int32 LastTick = 0;
			};

			Args args{ this };

			if (converter == nullptr)
				converter = createMidiConverter();

			converterSetTokenizer(converter, GetTok());

			int32 currentTick = Outputs.MidiClock->GetCurrentHiResTick();

			converterSetOnNote(converter, [](void* data, const Note& newNote)
				{
					Args& args = *(Args*)(data);

					int32 Channel = 1;
					int32 NoteNumber = newNote.pitch;
					int32 Velocity = newNote.velocity;

					int32 Track = 0;

					int32 CurrentTick = args.self->Outputs.MidiClock->GetCurrentHiResTick();

					int32 Tick = newNote.tick * 100 + args.self->AddedTicks;

					if (Tick < CurrentTick)
					{
						args.self->AddedTicks += CurrentTick - Tick;
						Tick = CurrentTick;
					}

					if (args.self->Generator->MidiFileData->Tracks[0].GetUnsortedEvents().IsEmpty() or Tick >= args.self->Generator->MidiFileData->Tracks[0].GetEvents().Last().GetTick())
					{
						args.LastTick = Tick;

						FMidiMsg Msg{ FMidiMsg::CreateNoteOn(Channel - 1, NoteNumber, Velocity) };
						args.self->Generator->MidiFileData->Tracks[0].AddEvent(FMidiEvent(Tick, Msg));
					}

				});

			int32* newDecodedTokens = nullptr;
			int32 newDecodedTokensSize = 0;
			tokenizer_decodeIDs(GetTok(), Generator->NewEncodedTokens.GetData(), Generator->NewEncodedTokens.Num(), &newDecodedTokens, &newDecodedTokensSize);
			Generator->NewEncodedTokens.Empty();
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
				}
			}

			ensureMsgf(!Generator->MidiFileData->Tracks[0].GetUnsortedEvents().IsEmpty(), TEXT("Should not be empty!"));
			//checkf((Index >= 0)& (Index < ArrayNum), TEXT("Array index out of bounds: %lld into an array of size %lld"), (long long)Index, (long long)ArrayNum); // & for one branch

			//Outputs.MidiClock->MidiDataChangesComplete(FMidiPlayCursorMgr::EMidiChangePositionCorrectMode::MaintainTick);

			//MidiFileData->Tracks[0].Sort(); // TOO SLOW, O(n*log(n))

			Outputs.MidiClock->GetDrivingMidiPlayCursorMgr()->MidiDataChangeComplete(FMidiPlayCursorMgr::EMidiChangePositionCorrectMode::MaintainTick);
		}

		~FAIGenOperator()
		{
			destroyMidiConverter(converter);
			if (Generator.IsValid())
			{
				OnRemoveGen();
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
			TryUpdateGenThreadInput();

			if ((!Generator.IsValid()) || GetTok() == nullptr || GetGen() == nullptr)
			{
				return;
			}

			int32 currentTick = Outputs.MidiClock->GetCurrentHiResTick();
			int32 lastTick = Generator->MidiFileData->Tracks[0].GetEvents().Last().GetTick();

			if (Generator->GenThread.IsValid() && bShouldUpdateTokens)
			{
				TokenModifSection.Lock();
				UpdateScheduledMidiEvents();
				bShouldUpdateTokens = false;
				TokenModifSection.Unlock();
			}

			// Block until music is generated
			while (Generator.IsValid() && Generator->GenThread.IsValid() && currentTick > lastTick)
			{
				if (bShouldUpdateTokens)
				{
					TokenModifSection.Lock();
					UpdateScheduledMidiEvents();
					bShouldUpdateTokens = false;
					TokenModifSection.Unlock();
				}
				lastTick = Generator->MidiFileData->Tracks[0].GetEvents().Last().GetTick();
			}

			Outputs.MidiStream->PrepareBlock();

			Outputs.MidiClock->PrepareBlock();

			InitTransportIfNeeded();

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

		//FMidiFileProxyPtr MidiDataProxy;
		//TSharedPtr<FMidiFileData> MidiFileData;

		//TArray<int32> NewEncodedTokens;
		TArray<int32> DecodedTokens;

		FSampleCount BlockSize = 0;
		int32 PrerollBars = 8;
		//TSharedPtr<FGenThread> GenThread;
		TSharedPtr<FMIDIGeneratorEnv> Generator;

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