// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "HarmonixMidi/MidiFile.h"
#include "MIDIGeneratorEnv.generated.h"

struct FMIDIGeneratorEnv;
class FMIDIGeneratorProxy;
using FMIDIGeneratorProxyPtr = TSharedPtr<FMIDIGeneratorProxy, ESPMode::ThreadSafe>;

class FMIDIGeneratorProxy final : public Audio::TProxyData<FMIDIGeneratorProxy>
{
public:
	IMPL_AUDIOPROXY_CLASS(FMIDIGeneratorProxy);

	explicit FMIDIGeneratorProxy(const TSharedPtr<FMIDIGeneratorEnv>& Data)
		: MidiGenerator(Data)
	{}

	//explicit FMIDIGeneratorProxy(FMIDIGeneratorEnv* Data)
	//	: MidiGenerator(Data)
	//{}

	TSharedPtr<FMIDIGeneratorEnv> GetMidiFile()
	{
		return MidiGenerator;
	}

	TSharedPtr<FMIDIGeneratorEnv> MidiGenerator;
};

struct MIDIGENERATORWRAPPER_API FMIDIGeneratorEnv
{
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 maxPitch = 127;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 minPitch = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 maxDeltaTime = 100000;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 minDeltaTime = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 maxIntensity = 100000;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 minIntensity = 0;

	TSharedPtr<class FGenThread> GenThread = MakeShared<FGenThread>();

	bool bShouldUpdateTokens = false;
	FCriticalSection TokenModifSection;

	TSharedPtr<struct FMidiFileData> MidiFileData;
	FMidiFileProxyPtr MidiDataProxy;

public:
	void StartGeneration();
	void StopGeneration();

	void PreStart(const FString& TokenizerPath, const FString& ModelPath, const TArray<int32>& InTokens);
};



/**
 * 
 */
UCLASS(BlueprintType)
class MIDIGENERATORWRAPPER_API UMIDIGeneratorEnv : public UObject
{
	GENERATED_BODY()

public:
	FMIDIGeneratorProxyPtr Generator;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 maxPitch = 127;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 minPitch = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 maxDeltaTime = 100000;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 minDeltaTime = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 maxIntensity = 100000;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 minIntensity = 0;
	
public:
	UFUNCTION(BlueprintCallable)
	void StartGeneration();

	UFUNCTION(BlueprintCallable)
	void StopGeneration();
};