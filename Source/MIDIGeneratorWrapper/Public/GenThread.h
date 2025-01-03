// Copyright Prog'z. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MIDIGenerator.h"
#include "TokenizerAsset.h"

DECLARE_CYCLE_STAT(TEXT("GenThread"), STAT_GenThread, STATGROUP_Game);

DECLARE_DELEGATE_OneParam(FOnGenerated, int32);

//class FGenThread;
//class FMIDIGeneratorProxy;
//using FMIDIGeneratorProxyPtr = TSharedPtr<FMIDIGeneratorProxy, ESPMode::ThreadSafe>;
//
//class FMIDIGeneratorProxy final : public Audio::TProxyData<FMIDIGeneratorProxy>
//{
//public:
//	IMPL_AUDIOPROXY_CLASS(FMIDIGeneratorProxy);
//
//	explicit FMIDIGeneratorProxy(const TSharedPtr<FGenThread>& Data)
//		: MidiGenerator(Data)
//	{}
//
//	//explicit FMIDIGeneratorProxy(FGenThread* Data)
//	//	: MidiGenerator(Data)
//	//{}
//
//	TSharedPtr<FGenThread> GetMidiFile()
//	{
//		return MidiGenerator;
//	}
//
//	TSharedPtr<FGenThread> MidiGenerator;
//};


/**
 * 
 */
class MIDIGENERATORWRAPPER_API FGenThread : public FRunnable//, public IAudioProxyDataFactory
{
public:
	//FGenThread(const FString& TokenizerPath, const FString& ModelPath);
	void PreStart(const FString& TokenizerPath, const FString& ModelPath, const TArray<int32>& InTokens);
	void Start(const FString& TokenizerPath, const FString& ModelPath, const TArray<int32>& InTokens);
	void Start();
	bool HasStarted() const;

	~FGenThread();

	MidiTokenizerHandle GetTok() const
	{
		return Tokenizer->GetTokenizer()->Tokenizer;
	}
	MusicGeneratorHandle GetGen() const
	{
		return generator;
	}

	static FString RelativeToAbsoluteContentPath(const FString& BaseStr);


	EnvHandle env;
	MusicGeneratorHandle generator;


	FString TokenizerPath; 
	FString ModelPath;
	//FMIDIGenerator Generator;

	FTokenizerProxyPtr Tokenizer = MakeShared<FTokenizerProxy>((MidiTokenizerHandle)nullptr);

	void* SearchStrategyData = nullptr;
	TSearchStrategy SearchStrategy = nullptr;

	RunInstanceHandle runInstance;
	BatchHandle batch;

	TArray<int32> EncodedLine;
	//int32 LineNbMaxToken = 256;
	int32 LineNbMaxToken = 511;
	int32 NbMaxTokensAhead = 50;

	int32 NbBatchGen = 10;

	bool forceReupdate = false;


	int32 NextTokenIndexToPlay = 0; // @TODO : Update from game thread and make thread safe

	TArray<int32> TokenGroupToInsert;
	int32 TokenGroupInsertTick = 10;

	// @TODO : Thread safe
	void AddTokenGroupToInsert(const TArray<int32>& TokenGroup);
	void TryInsertTokenGroup();

	virtual bool Init() override;
	virtual uint32 Run() override;
	virtual void Exit() override;
	virtual void Stop() override;

	FRunnableThread* Thread;
	bool bShutdown = false;

	FOnGenerated OnGenerated;

	////~Begin IAudioProxyDataFactory Interface.
	//virtual TSharedPtr<Audio::IProxyData> CreateProxyData(const Audio::FProxyDataInitParams& InitParams) override;
	////~ End IAudioProxyDataFactory Interface.
};