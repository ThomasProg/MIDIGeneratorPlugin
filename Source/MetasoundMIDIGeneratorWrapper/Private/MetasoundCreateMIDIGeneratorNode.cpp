// Copyright Epic Games, Inc. All Rights Reserved.

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
#include "MIDIGenerator.h"
#include "MetasoundMIDIGenerator.h"

#include "MetasoundExecutableOperator.h"     // TExecutableOperator class
#include "MetasoundPrimitives.h"             // ReadRef and WriteRef descriptions for bool, int32, float, and string
#include "MetasoundNodeRegistrationMacro.h"  // METASOUND_LOCTEXT and METASOUND_REGISTER_NODE macros
#include "MetasoundStandardNodesNames.h"     // StandardNodes namespace
#include "MetasoundFacade.h"				         // FNodeFacade class, eliminates the need for a fair amount of boilerplate code
#include "MetasoundParamHelper.h"            // METASOUND_PARAM and METASOUND_GET_PARAM family of macros

// Required for ensuring the node is supported by all languages in engine. Must be unique per MetaSound.
#define LOCTEXT_NAMESPACE "MetasoundStandardNodes_MetaSoundSoundfontMIDIStreamPlayerNode"

namespace Metasound
{
	// Vertex Names - define your node's inputs and outputs here
	namespace CreateMIDIGeneratorNodeNames
	{
		METASOUND_PARAM(InModelPath, "ModelPath", "The model used");
		METASOUND_PARAM(InTokenizerPath, "TokenizerPath", "The tokenizer used");
		METASOUND_PARAM(InStartTokens, "StartTokens", "The tokens used at the start of the generation");

		METASOUND_PARAM(OutGenerator, "Generator", "The synth created");
	}


	// Operator Class - defines the way your node is described, created and executed
	class FCreateMIDIGeneratorOperator : public TExecutableOperator<FCreateMIDIGeneratorOperator>
	{
	public:
		// Constructor
		FCreateMIDIGeneratorOperator(const Metasound::FBuildOperatorParams& InParams,
			FStringReadRef ModelPath,
			FStringReadRef TokenizerPath,
			const TArray<int32>& StartTokens)
			:  Inputs{ ModelPath, TokenizerPath, StartTokens }
			, Outputs{ TDataWriteReferenceFactory<Metasound::FMIDIGeneratorZZZ>::CreateAny(InParams.OperatorSettings) }
		{
			UpdateOutputs();
		}

		// Helper function for constructing vertex interface
		static const FVertexInterface& DeclareVertexInterface()
		{
			using namespace CreateMIDIGeneratorNodeNames;

			static const FVertexInterface Interface(
				FInputVertexInterface(
					TInputDataVertex<FString>(METASOUND_GET_PARAM_NAME_AND_METADATA(InModelPath)),
					TInputDataVertex<FString>(METASOUND_GET_PARAM_NAME_AND_METADATA(InTokenizerPath)),
					TInputDataVertex<TArray<int32>>(METASOUND_GET_PARAM_NAME_AND_METADATA(InStartTokens))
					//TInputDataVertex<HarmonixMetasound::FMidiStream>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputMidiStream)),
					//TInputDataVertex<Metasound::FGenerator>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputGenerator)),
					//TInputDataVertex<FTrigger>("Play", FDataVertexMetadata{ LOCTEXT("MetaSoundSoundfontPlayerNode_InputPlayDesc", "Plays the given note") })
				),
				FOutputVertexInterface(
					//TOutputDataVertex<Metasound::FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputAudioLeft)),
					//TOutputDataVertex<Metasound::FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputAudioRight))
					TOutputDataVertex<Metasound::FMIDIGeneratorZZZ>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutGenerator))
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
						FNodeClassName { StandardNodes::Namespace, "CreateMIDIGenerator Node", StandardNodes::AudioVariant },
						1, // Major Version
						0, // Minor Version
						METASOUND_LOCTEXT("CreateMIDIGeneratorNodeDisplayName", "CreateMIDIGenerator Node"),
						METASOUND_LOCTEXT("CreateMIDIGeneratorNodeDesc", "Creates a Synth Instance to be used by SoundfontMIDIPlayer"),
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
			FStringReadRef ModelPath;
			FStringReadRef TokenizerPath;
			TArray<int32> StartTokens;
		};
		
		struct FOutputs
		{
			Metasound::FMIDIGeneratorZZZWriteRef Generator;
		};


		virtual void BindInputs(FInputVertexInterfaceData& InOutVertexData) override
		{
			InOutVertexData.BindReadVertex("ModelPath", Inputs.ModelPath);
			InOutVertexData.BindReadVertex("TokenizerPath", Inputs.TokenizerPath);
			InOutVertexData.SetValue(METASOUND_GET_PARAM_NAME(CreateMIDIGeneratorNodeNames::InStartTokens), Inputs.StartTokens);
		}

		virtual void BindOutputs(FOutputVertexInterfaceData& InOutVertexData) override
		{
			InOutVertexData.BindWriteVertex("Generator", TDataWriteReference<FMIDIGeneratorZZZ>(Outputs.Generator));
		}

		static TUniquePtr<Metasound::IOperator> CreateOperator(const FBuildOperatorParams& InParams, FBuildResults& OutResults)
		{
			const FOperatorSettings& Settings = InParams.OperatorSettings;
			const FInputVertexInterfaceData& InputData = InParams.InputData;

			FStringReadRef ModelPath = InputData.GetOrConstructDataReadReference<FString>("ModelPath");
			FStringReadRef TokenizerPath = InputData.GetOrConstructDataReadReference<FString>("TokenizerPath");
			TArray<int32> StartTokens = InputData.GetOrCreateDefaultValue<TArray<int32>>(METASOUND_GET_PARAM_NAME(CreateMIDIGeneratorNodeNames::InStartTokens), InParams.OperatorSettings);

			return MakeUnique<FCreateMIDIGeneratorOperator>(InParams, ModelPath, TokenizerPath, StartTokens);
		}

public:
		void UpdateOutputs()
		{
			if (!MIDIGenerator.IsValid())
			{
				MIDIGenerator = MakeShared<FGenThread>();

				auto GetPath = [](const FString& BaseStr) -> FString
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
					};

				MIDIGenerator->Start(GetPath(*Inputs.TokenizerPath), GetPath(*Inputs.ModelPath), Inputs.StartTokens);
			}
			const FMIDIGeneratorZZZ& Inst = *Outputs.Generator;
			FMIDIGeneratorProxyPtr Proxy = Inst.GetProxy();
			Proxy->MidiGenerator = MIDIGenerator;
		}

		// Primary node functionality
		void Execute()
		{
			UpdateOutputs();

		}

	private:
		FInputs Inputs;
		FOutputs Outputs;

		TSharedPtr<FGenThread> MIDIGenerator;
	};

	// Node Class - Inheriting from FNodeFacade is recommended for nodes that have a static FVertexInterface
	class FCreateMIDIGeneratorNode : public FNodeFacade
	{
	public:
		FCreateMIDIGeneratorNode(const FNodeInitData& InitData)
			: FNodeFacade(InitData.InstanceName, InitData.InstanceID, TFacadeOperatorClass<FCreateMIDIGeneratorOperator>())
		{
		}
	};






	// Register node
	METASOUND_REGISTER_NODE(FCreateMIDIGeneratorNode);
}
















#undef LOCTEXT_NAMESPACE