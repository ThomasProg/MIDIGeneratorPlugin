// Copyright Prog'z. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MIDIGenerator.h"
#include "TokenizerAsset.h"
#include "fwd.h"

DECLARE_CYCLE_STAT(TEXT("GenThread"), STAT_GenThread, STATGROUP_Game);

DECLARE_DELEGATE_OneParam(FOnGenerated, int32);
DECLARE_DELEGATE_OneParam(FOnSearch, const struct SearchArgs& args);
DECLARE_DELEGATE(FOnInit);

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
	void SetPipeline(IAutoRegressivePipeline* NewPipeline);
	void PreStart(const FString& TokenizerPath, const FString& ModelPath, const TArray<int32>& InTokens);
	void Start(const FString& TokenizerPath, const FString& ModelPath, const TArray<int32>& InTokens);
	void Start();
	bool HasStarted() const;

	void SetTokens(const TArray<int32>& InTokens);

	~FGenThread();

	//MidiTokenizerHandle GetTok() const
	//{
	//	return Tokenizer->GetTokenizer()->Tokenizer;
	//}

	void SetTok(const FTokenizerProxyPtr& InTokenizer)
	{
		Tokenizer = InTokenizer;
	}

	const FTokenizer& GetTok() const
	{
		return *Tokenizer->GetTokenizer();
	}

	MusicGeneratorHandle GetGen() const
	{
		return generator;
	}

	void GetEncodedTokens(TArray<int32>& OutEncodedTokens)
	{
		Mutex.Lock();
		OutEncodedTokens = EncodedTokens;
		Mutex.Unlock();
	}

	static FString RelativeToAbsoluteContentPath(const FString& BaseStr);

	//void SetSearchStrategy(void* SearchStrategyData, TSearchStrategy SearchStrategy);
	void SetSearchStrategy(FOnSearch InOnSearch);
	void SetOnGenerated(FOnGenerated InOnGenerated);
	void SetOnInit(FOnInit InOnInit);

	// BEGIN FRunnable 
	virtual void Stop() override;
	// END FRunnable

protected:
	// BEGIN FRunnable 
	virtual bool Init() override;
	virtual uint32 Run() override;
	virtual void Exit() override;
	// END FRunnable

private:
	IAutoRegressivePipeline* Pipeline = nullptr;
	EnvHandle env;
	MusicGeneratorHandle generator;

	FString TokenizerPath; 
	FString ModelPath;

	FTokenizerProxyPtr Tokenizer;

	//void* SearchStrategyData = nullptr;
	//TSearchStrategy SearchStrategy = nullptr;
	FOnSearch OnSearch;
	FOnGenerated OnGenerated;
	FOnInit OnInit;
	FCriticalSection Mutex;

	RunInstanceHandle runInstance;
	BatchHandle batch;
	AutoRegressiveBatchHandle Batch2;

	TArray<int32> EncodedTokens;
	//int32 LineNbMaxToken = 256;
	//int32 LineNbMaxToken = 512;
	int32 LineNbMaxToken = 1024;
	int32 NbMaxTokensAhead = 50;

	int32 NbBatchGen = 10;

	bool forceReupdate = false;

	FRunnableThread* Thread;
	bool bShutdown = false;



	int32 NbTokensSinceLastRefresh = 0;

	////~Begin IAudioProxyDataFactory Interface.
	//virtual TSharedPtr<Audio::IProxyData> CreateProxyData(const Audio::FProxyDataInitParams& InitParams) override;
	////~ End IAudioProxyDataFactory Interface.
};