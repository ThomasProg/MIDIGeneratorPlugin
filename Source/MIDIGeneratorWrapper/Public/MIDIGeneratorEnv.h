// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "HarmonixMidi/MidiFile.h"
#include "IAudioProxyInitializer.h"
#include "fwd.h"
#include "HarmonixMetasound/DataTypes/MidiClock.h"
#include "MIDIGeneratorEnv.generated.h"

struct FMIDIGeneratorEnv;
class FMIDIGeneratorProxy;
using FMIDIGeneratorProxyPtr = TSharedPtr<FMIDIGeneratorProxy, ESPMode::ThreadSafe>;

DECLARE_CYCLE_STAT(TEXT("GenThread::LogitProcessing1"), STAT_GenThread_LogitProcessing1, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("GenThread::LogitProcessing2"), STAT_GenThread_LogitProcessing2, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("GenThread::LogitProcessing3"), STAT_GenThread_LogitProcessing3, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("GenThread::LogitProcessing4"), STAT_GenThread_LogitProcessing4, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("GenThread::LogitProcessing5"), STAT_GenThread_LogitProcessing5, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("GenThread::LogitProcessing6"), STAT_GenThread_LogitProcessing6, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("GenThread::LogitProcessing7"), STAT_GenThread_LogitProcessing7, STATGROUP_Game);

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

// Pipeline
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
	int32 nbSkips = 0;

	int32 nextNoteIndexToProcess = 0;

	RangeGroupHandle BaseRangeGroup;
	RangeGroupHandle PitchRangeGroup;
	RangeGroupHandle VelocityRangeGroup;
	RangeGroupHandle DurationRangeGroup;
	RangeGroupHandle TimeShiftRangeGroup;
	RangeGroupHandle AllRangeGroup;

	const HarmonixMetasound::FMidiClock* Clock = nullptr;

public:
	~FMIDIGeneratorEnv();
	void StartGeneration();
	void StopGeneration();

	// @TODO : remove
	// should be initialized with a TokenizerAsset and a ModelAsset instead
	void PreStart(const FString& TokenizerPath, const FString& ModelPath, const TArray<int32>& InTokens);
	void PreloadPipeline(const FString& ModelPath);
	void SetTokens(const TArray<int32>& InTokens);

	void SetFilter();
	void DecodeTokens();

	void SetClock(const HarmonixMetasound::FMidiClock& InClock);
	void RegenerateCacheAfterDelay(float DelayInMs);
	void UpdateCurrentRangeGroup(int32 LastDecodedToken);

	int32 UETickToGenLibTick(float tick);
	float GenLibTickToUETick(int32 tick);
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
	void PreloadPipeline(const FString& ModelPath);

	UFUNCTION(BlueprintCallable)
	void SetFilter();

	UFUNCTION(BlueprintCallable)
	void SetTokenizer(class UTokenizerAsset* InTokenizer);

	UFUNCTION(BlueprintCallable)
	void SetTokens(const TArray<int32>& InTokens);

	UFUNCTION(BlueprintCallable)
	void SetTempo(float InTempo);

	UFUNCTION(BlueprintCallable)
	void RegenerateCacheAfterDelay(float DelayInMs);

	//~Begin IAudioProxyDataFactory Interface.
	virtual TSharedPtr<Audio::IProxyData> CreateProxyData(const Audio::FProxyDataInitParams& InitParams) override;
	//~ End IAudioProxyDataFactory Interface.
};
