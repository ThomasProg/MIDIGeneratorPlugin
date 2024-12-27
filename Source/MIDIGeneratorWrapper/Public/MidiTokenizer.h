// Copyright Prog'z. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "gen.h"
#include "HarmonixMidi/MidiMsg.h"

/**
 * 
 */
class MIDIGENERATORWRAPPER_API FMidiConverter
{
public:
	FMidiConverter();
	~FMidiConverter();


	MidiTokenizerHandle tok;
	RedirectorHandle redirector;

	TArray<int32> tokens;
	int32 unplayedTokenIndex = 0;


	void update();


};
