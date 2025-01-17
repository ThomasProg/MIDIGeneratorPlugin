// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "HarmonixMidi/MidiFile.h"
#include "IAudioProxyInitializer.h"
#include "fwd.h"
#include "MIDIGeneratorEnv.generated.h"

struct FMIDIGeneratorEnv;
class FMIDIGeneratorProxy;
using FMIDIGeneratorProxyPtr = TSharedPtr<FMIDIGeneratorProxy, ESPMode::ThreadSafe>;

class FMIDIGeneratorProxy final : public Audio::TProxyData<FMIDIGeneratorProxy>
{
public:
	IMPL_AUDIOPROXY_CLASS(FMIDIGeneratorProxy);

	//explicit FMIDIGeneratorProxy(const TSharedPtr<FMIDIGeneratorEnv>& Data)
	//	: MidiGenerator(Data)
	//{}

	MIDIGENERATORWRAPPER_API explicit FMIDIGeneratorProxy(UMIDIGeneratorEnv* InGeneratorEnv);
	MIDIGENERATORWRAPPER_API explicit FMIDIGeneratorProxy(const TSharedPtr<FMIDIGeneratorEnv>& InGeneratorEnv);

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

	TArray<int32> NewEncodedTokens;
	bool bShouldUpdateTokens = false;
	FCriticalSection EncodedTokensSection;
	TArray<int32> DecodedTokens;

	TSharedPtr<struct FMidiFileData> MidiFileData;
	FMidiFileProxyPtr MidiDataProxy;

	MidiConverterHandle converter = nullptr;
	RangeGroupHandle CurrentRangeGroup = nullptr;

	int32 CurrentTick = 0;
	int32 AddedTicks = 0;
	int32 nextTokenToProcess = 0;

	RangeGroupHandle BaseRangeGroup;
	RangeGroupHandle PitchRangeGroup;
	RangeGroupHandle VelocityRangeGroup;
	RangeGroupHandle DurationRangeGroup;
	RangeGroupHandle AllRangeGroup;

public:
	~FMIDIGeneratorEnv();
	void StartGeneration();
	void StopGeneration();

	void PreStart(const FString& TokenizerPath, const FString& ModelPath, const TArray<int32>& InTokens);

	void SetFilter();
	void DecodeTokens();
};



/**
 * 
 */
UCLASS(BlueprintType)
class MIDIGENERATORWRAPPER_API UMIDIGeneratorEnv : public UObject, public IAudioProxyDataFactory
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
	UMIDIGeneratorEnv();

	UFUNCTION(BlueprintCallable)
	void StartGeneration();

	UFUNCTION(BlueprintCallable)
	void StopGeneration();

	UFUNCTION(BlueprintCallable)
	void PreStart(const FString& TokenizerPath, const FString& ModelPath, const TArray<int32>& InTokens);

	UFUNCTION(BlueprintCallable)
	void SetFilter();

	UFUNCTION(BlueprintCallable)
	void SetTokenizer(class UTokenizerAsset* InTokenizer);

	//~Begin IAudioProxyDataFactory Interface.
	virtual TSharedPtr<Audio::IProxyData> CreateProxyData(const Audio::FProxyDataInitParams& InitParams) override;
	//~ End IAudioProxyDataFactory Interface.
};
