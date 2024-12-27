// Copyright Prog'z. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"

class FMetasoundMIDIGeneratorWrapperModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:

};
