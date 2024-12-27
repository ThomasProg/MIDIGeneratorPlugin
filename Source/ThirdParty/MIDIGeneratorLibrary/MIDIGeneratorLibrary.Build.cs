// Copyright Epic Games, Inc. All Rights Reserved.

using Microsoft.Extensions.Logging;
using System.IO;
using UnrealBuildTool;

public class MIDIGeneratorLibrary : ModuleRules
{
	public string BinDir { get => "MIDIGenerator\\bin"; }
	public string IncludeDir { get => "MIDIGenerator\\include"; }
	public string LibDir { get => "MIDIGenerator\\lib"; }

	public MIDIGeneratorLibrary(ReadOnlyTargetRules Target) : base(Target)
	{
		Type = ModuleType.External;

		PublicIncludePaths.Add(Path.Combine(ModuleDirectory, IncludeDir));
		PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, LibDir, "MidiTokCpp.lib"));

		string[] str = {
			"MidiTokCpp.dll",
			"onnxruntime.dll",
		};

		string OutputBinariesDir = "";

		// Ensure the DLL is copied to the right binaries folder
		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
            OutputBinariesDir = Path.Combine(ModuleDirectory, "..", "..", "Binaries", "Win64");
        }

        System.Diagnostics.Debug.Assert(!string.IsNullOrEmpty(OutputBinariesDir), "BinariesDir should not be empty!");

        foreach (string s in str)
		{
			PublicDelayLoadDLLs.Add(s);

            string DestDLLPath = Path.Combine(ModuleDirectory, OutputBinariesDir, s);
            string SrcDLLPath = Path.Combine(ModuleDirectory, BinDir, s);
            RuntimeDependencies.Add(DestDLLPath, SrcDLLPath);

            // include it in packaged builds
            string PackagedDestDLLPath = Path.Combine("$(BinaryOutputDir)", s);
            RuntimeDependencies.Add(PackagedDestDLLPath, SrcDLLPath);

            Logger.LogTrace(Path.Combine(BinDir, s));
		}
	}
}
