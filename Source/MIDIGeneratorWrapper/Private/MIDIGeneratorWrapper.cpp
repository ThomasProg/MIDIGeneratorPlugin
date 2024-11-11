// Copyright Epic Games, Inc. All Rights Reserved.

#include "MIDIGeneratorWrapper.h"
#include "Misc/MessageDialog.h"
#include "Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "HAL/PlatformProcess.h"

#define LOCTEXT_NAMESPACE "FMIDIGeneratorWrapperModule"

void FMIDIGeneratorWrapperModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

	// Get the base directory of this plugin
	FString BaseDir = IPluginManager::Get().FindPlugin("MIDIGeneratorWrapper")->GetBaseDir();

	// Add on the relative location of the third party dll and load it
	FString LibraryPath;
#if PLATFORM_WINDOWS
	//FString s = "C:/Users/thoma/PandorasBox/Projects/ModularMusicGenerationModules/Modules/RuntimeModules/fluidsynth/Downloads/bin";
	FString s = FPaths::ProjectPluginsDir() + "/MIDIGeneratorPlugin/Source/ThirdParty/MIDIGeneratorLibrary/MIDIGenerator/bin";
	LibraryPath = FPaths::Combine(*s, TEXT("MidiTokCpp.dll"));


//#elif PLATFORM_MAC
//	LibraryPath = FPaths::Combine(*BaseDir, TEXT("Source/ThirdParty/MIDIGeneratorLibrary/Mac/Release/libExampleLibrary.dylib"));
//#elif PLATFORM_LINUX
//	LibraryPath = FPaths::Combine(*BaseDir, TEXT("Binaries/ThirdParty/MIDIGeneratorLibrary/Linux/x86_64-unknown-linux-gnu/libExampleLibrary.so"));
#endif // PLATFORM_WINDOWS

	ExampleLibraryHandle = !LibraryPath.IsEmpty() ? FPlatformProcess::GetDllHandle(*LibraryPath) : nullptr;

	if (ExampleLibraryHandle)
	{
		// Call the test function in the third party library that opens a message box
		//ExampleLibraryFunction();
	}
	else
	{
		FText ErrorMessage = FText::Format(
			LOCTEXT("ThirdPartyLibraryError", "MIDIGeneratorWrapper: Failed to load the library at path: {0}"),
			FText::FromString(LibraryPath)
		);
		FMessageDialog::Open(EAppMsgType::Ok, ErrorMessage);
	}
}

void FMIDIGeneratorWrapperModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	// Free the dll handle
	FPlatformProcess::FreeDllHandle(ExampleLibraryHandle);
	ExampleLibraryHandle = nullptr;
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FMIDIGeneratorWrapperModule, MIDIGeneratorWrapper)
