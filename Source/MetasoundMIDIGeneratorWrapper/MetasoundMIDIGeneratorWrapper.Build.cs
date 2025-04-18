// Copyright Prog'z. All Rights Reserved.

using UnrealBuildTool;

public class MetasoundMIDIGeneratorWrapper : ModuleRules
{
	public MetasoundMIDIGeneratorWrapper(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		OptimizeCode = CodeOptimization.Never;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"MIDIGeneratorLibrary",
				"Projects",
                "MetasoundEngine",
                "MetasoundFrontend",
                "MetasoundStandardNodes",
                "HarmonixMetasound",
                "HarmonixDsp",
                "HarmonixMidi",
                "Harmonix",
                "Serialization",
				"MIDIGeneratorWrapper",
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				// ... add private dependencies that you statically link with here ...	
					"AudioExtensions",
					"AudioMixer",
					"SignalProcessing",
                    "MetasoundGraphCore",
                    "MetasoundGenerator",
            }
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
