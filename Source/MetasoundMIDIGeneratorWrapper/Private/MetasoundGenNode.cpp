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

#include "MIDIGenerator.h"

// Required for ensuring the node is supported by all languages in engine. Must be unique per MetaSound.
#define LOCTEXT_NAMESPACE "MetasoundStandardNodes_MetaSoundAIGenNode"



namespace AIGen
{
	namespace Inputs
	{
		METASOUND_PARAM(Transport, "Transport", "The synthesized right audio channel.");
	}

	namespace Outputs
	{
		METASOUND_PARAM(MidiStream, "MidiStream", "MidiStream that is synthesized.");
	}
}


namespace Metasound
{
	// Operator Class - defines the way your node is described, created and executed
	class FAIGenOperator : public TExecutableOperator<FAIGenOperator>, public FMidiPlayCursor, public FMidiVoiceGeneratorBase, public HarmonixMetasound::FMusicTransportControllable
	{
	public:
		// Constructor
		FAIGenOperator(const Metasound::FBuildOperatorParams& InParams,
			const HarmonixMetasound::FMusicTransportEventStreamReadRef& InTransport)
			: FMusicTransportControllable(HarmonixMetasound::EMusicPlayerTransportState::Prepared)
			, Inputs{ InTransport }
			, Outputs{ HarmonixMetasound::FMidiStreamWriteRef::CreateNew() }
		{
			//Outputs.MidiStream->SetClock(*MidiClockOut);
			if (Generator == nullptr)
			{
				Generator = NewObject<UMIDIGenerator>();
			}

			Generator->Init();




			Generator->Generate(30, TokensToPlay);
		}

		~FAIGenOperator()
		{
			if (Generator != nullptr)
			{
				Generator->Deinit();
			}
		}

		// Helper function for constructing vertex interface
		static const FVertexInterface& DeclareVertexInterface()
		{
			static const FVertexInterface Interface(
				FInputVertexInterface(
					TInputDataVertex<HarmonixMetasound::FMusicTransportEventStream>(METASOUND_GET_PARAM_NAME_AND_METADATA(AIGen::Inputs::Transport))
				),
				FOutputVertexInterface(
					TOutputDataVertex<HarmonixMetasound::FMidiStream>(METASOUND_GET_PARAM_NAME_AND_METADATA(AIGen::Outputs::MidiStream))
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
			HarmonixMetasound::FMusicTransportEventStreamReadRef Transport;
		};

		struct FOutputs
		{
			HarmonixMetasound::FMidiStreamWriteRef MidiStream;
		};


		virtual void BindInputs(FInputVertexInterfaceData& InOutVertexData) override
		{
			InOutVertexData.BindReadVertex("Transport", Inputs.Transport);
		}

		virtual void BindOutputs(FOutputVertexInterfaceData& InOutVertexData) override
		{
			InOutVertexData.BindReadVertex("MidiStream", Outputs.MidiStream);
		}



		// Allows MetaSound graph to interact with your node's inputs
		virtual FDataReferenceCollection GetInputs() const override
		{
			FDataReferenceCollection InputDataReferences;

			InputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(AIGen::Inputs::Transport), Inputs.Transport);

			return InputDataReferences;
		}

		// Allows MetaSound graph to interact with your node's outputs
		virtual FDataReferenceCollection GetOutputs() const override
		{
			FDataReferenceCollection OutputDataReferences;

			OutputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(AIGen::Outputs::MidiStream), Outputs.MidiStream);

			return OutputDataReferences;
		}

		static TUniquePtr<Metasound::IOperator> CreateOperator(const FBuildOperatorParams& InParams, FBuildResults& OutResults)
		{
			const FOperatorSettings& Settings = InParams.OperatorSettings;
			const FInputVertexInterfaceData& InputData = InParams.InputData;
			HarmonixMetasound::FMusicTransportEventStreamReadRef InTransport = InputData.GetOrConstructDataReadReference<HarmonixMetasound::FMusicTransportEventStream>(METASOUND_GET_PARAM_NAME(AIGen::Inputs::Transport), Settings);
			return MakeUnique<FAIGenOperator>(InParams, InTransport);
		}


	public:
		// Primary node functionality
		void Execute()
		{
			if (TokensToPlay.IsEmpty())
				return;

			// try to convert and extract
			bool isValid = false;
			int32 index = 0;
			while (index < TokensToPlay.Num() && !isValid)
			{
				int32 token = TokensToPlay[index];

				// @TODO : Consider token and convert that to a token

				index++;
			}

			if (!isValid)
				return;
				
			for (int32 i = 0; i < index; i++)
			{
				TokensToPlay.RemoveAt(i, EAllowShrinking::No);
			}










			//HarmonixMetasound::FMidiStream& OutStream = *Outputs.MidiStream;

			//OutStream.PrepareBlock();

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
			//		OutStream.InsertMidiEvent(Event);

			//		LastNoteOn.Reset();
			//	}

			//	// Note on
			//	if (Enabled)
			//	{
			//		FMidiMsg Msg{ FMidiMsg::CreateNoteOn(Channel - 1, NoteNumber, Velocity) };
			//		HarmonixMetasound::FMidiStreamEvent Event{ &VoiceGenerator, Msg };
			//		Event.BlockSampleFrameIndex = NoteOnSample;
			//		Event.AuthoredMidiTick = NextPulse.MidiTick;
			//		Event.CurrentMidiTick = NextPulse.MidiTick;
			//		Event.TrackIndex = Track;
			//		OutStream.InsertMidiEvent(Event);

			//		LastNoteOn.Emplace(MoveTemp(Event));
			//	}
			//}
		}

	private:
		FInputs Inputs;
		FOutputs Outputs;

		TWeakObjectPtr<UMIDIGenerator> Generator = nullptr;
		TArray<int32> TokensToPlay;
	};

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