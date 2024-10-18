// Copyright Epic Games, Inc. All Rights Reserved.

#include "MetasoundMIDIGeneratorWrapper.h"
#include "Misc/MessageDialog.h"
#include "Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "HAL/PlatformProcess.h"
#include "MetasoundFrontendRegistries.h"

#define LOCTEXT_NAMESPACE "FMetasoundMIDIGeneratorWrapperModule"

void FMetasoundMIDIGeneratorWrapperModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	FMetasoundFrontendRegistryContainer::Get()->RegisterPendingNodes();
}

void FMetasoundMIDIGeneratorWrapperModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FMetasoundMIDIGeneratorWrapperModule, MetasoundMIDIGeneratorWrapper)
