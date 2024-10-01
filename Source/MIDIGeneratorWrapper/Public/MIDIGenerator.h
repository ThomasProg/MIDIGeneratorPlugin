// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "MIDIGenerator.generated.h"

/**
 * 
 */
UCLASS(Blueprintable, BlueprintType)
class MIDIGENERATORWRAPPER_API UMIDIGenerator : public UObject
{
	GENERATED_BODY()

	//virtual void PostInitProperties() override;
	//virtual void BeginDestroy() override;

	//UFUNCTION(BlueprintCallable)
	//void LoadSoundfont(const FString& path = "C:/Users/thoma/PandorasBox/Projects/ModularMusicGenerationModules/Assets/Soundfonts/Touhou/Touhou.sf2");
	//
	//UFUNCTION(BlueprintCallable)
	//void NoteOn(int32 channel, int32 key, int32 velocity);

	//UFUNCTION(BlueprintCallable)
	//void NoteOff(int32 channel, int32 key);

	UFUNCTION(BlueprintCallable)
	void Generate();
};
