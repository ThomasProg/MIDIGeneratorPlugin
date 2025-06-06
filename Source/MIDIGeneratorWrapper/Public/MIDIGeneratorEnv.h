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

DECLARE_CYCLE_STAT(TEXT("GenThread::DecodeToken1"), STAT_GenThread_DecodeToken1, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("GenThread::DecodeToken2"), STAT_GenThread_DecodeToken2, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("GenThread::DecodeToken3"), STAT_GenThread_DecodeToken3, STATGROUP_Game);

UENUM(BlueprintType) // This makes the enum usable in Blueprints
enum class EScale : uint8
{
	IonianMajor,
	//DorianMinor,
	Mixolydian,
	MelodicMinor,
	HarmonicMinor,
	WholeTone,
	Blues,
	PentatonicMajor,
	PentatonicMinor,
	HungarianMinor,
	Byzantine,
	Diminished
};

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
	int32 maxPitch = 60;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 minPitch = 40;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float maxTimeShift = 2.0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float minTimeShift = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 maxIntensity = 100000;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 minIntensity = 0;

	bool GenerateBeats = true;
	bool PlayFireworkEffect = false;

	TSharedPtr<class FGenThread> GenThread = MakeShared<FGenThread>();

	TArray<int32> NewEncodedTokens;
	bool bShouldUpdateTokens = false;
	TArray<int32> DecodedTokens;

	TSharedPtr<struct FMidiFileData> MidiFileData;
	FMidiFileProxyPtr MidiDataProxy;

	MidiConverterHandle converter = nullptr;
	RangeGroupHandle CurrentRangeGroup = nullptr;

	int32 CurrentTick = 0;
	int32 AddedTicks = 0;

	int32 nextNoteIndexToProcess = 0;
	int32 nextBeatNoteIndexToProcess = 0;

	RangeGroupHandle BaseRangeGroup;
	RangeGroupHandle PitchTimeshiftRangeGroup;
	RangeGroupHandle PitchRangeGroup;
	RangeGroupHandle VelocityRangeGroup;
	RangeGroupHandle DurationRangeGroup;
	RangeGroupHandle TimeShiftRangeGroup;
	RangeGroupHandle AllRangeGroup;

	const HarmonixMetasound::FMidiClock* Clock = nullptr;
	FCriticalSection ClockLock;


	bool hasRegen = false;
	int32 nbEncodedTokensSinceRegen = 0;
	int nbAddedSinceLastTimeshift = 0;
	int nbAddedSinceLast = 0;

	float CacheRemoveTick = 0;
	int32 LastFireworkPitch = 40;

	// @TODO : move to music director
	int32 callbackHash = -1;
	float callbackTime = 0;

	const int32_t* Scale = nullptr;
	int32_t ScaleSize = 0;

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

	void SetTempo(float InTempo);
	void RegenerateCacheFromTick(int32 UETick);
	void RegenerateCacheFromTick(int32 UETick, int32 LibTick);
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

	UFUNCTION(BlueprintCallable)
	void SetScale(EScale Scale);

	UFUNCTION(BlueprintCallable)
	void SetPitchRange(int32 MinPitch, int32 MaxPitch);

	UFUNCTION(BlueprintCallable)
	void GetPitchRange(int32& OutMinPitch, int32& OutMaxPitch) const;

	UFUNCTION(BlueprintCallable)
	void SetTimeShiftRange(float MinTimeShift, float MaxTimeShift);

	UFUNCTION(BlueprintCallable)
	void GetTimeShiftRange(float& OutMinTimeShift, float& OutMaxTimeShift) const;

	UFUNCTION(BlueprintCallable)
	void SetGenerateBeats(bool doesGenerate);

	UFUNCTION(BlueprintCallable)
	void SetPlayFireworkEffect(bool shouldPlayEffect);

	//~Begin IAudioProxyDataFactory Interface.
	virtual TSharedPtr<Audio::IProxyData> CreateProxyData(const Audio::FProxyDataInitParams& InitParams) override;
	//~ End IAudioProxyDataFactory Interface.
};
