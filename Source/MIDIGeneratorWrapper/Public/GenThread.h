// Copyright Prog'z. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MIDIGenerator.h"
#include "TokenizerAsset.h"
#include "fwd.h"

DECLARE_CYCLE_STAT(TEXT("GenThread"), STAT_GenThread, STATGROUP_Game);

DECLARE_MULTICAST_DELEGATE_OneParam(FOnGenerated, int32 newToken);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnSearch, const struct SearchArgs& args);
DECLARE_MULTICAST_DELEGATE(FOnInit);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnCacheRemoved, int32 libTick);

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
	IAutoRegressivePipeline* GetPipeline() const { return Pipeline; }
	AutoRegressiveBatchHandle GetBatch() const { return Batch2; }
	void PreStart(const FString& TokenizerPath, const FString& ModelPath, const TArray<int32>& InTokens);
	void Start(const FString& TokenizerPath, const FString& ModelPath, const TArray<int32>& InTokens);
	void Start();
	bool HasStarted() const;

	void SetTokens(const TArray<int32>& InTokens);

	FGenThread();
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
	void SetSearchStrategy(TFunction<void(const struct SearchArgs& args)> InOnSearch);
	void SetOnGenerated(TFunction<void(int32 NewToken)> InOnGenerated);
	void SetOnInit(TFunction<void()> InOnInit);

	// BEGIN FRunnable 
	virtual void Stop() override;
	// END FRunnable

	void RemoveCacheAfterTick(int32 GenLibTick, float Ms = -1.0);

protected:
	// BEGIN FRunnable 
	virtual bool Init() override;
	virtual uint32 Run() override;
	virtual void Exit() override;
	// END FRunnable

	void RemoveCacheAfterTickInternal();

private:
	IAutoRegressivePipeline* Pipeline = nullptr;
	EnvHandle env = nullptr;
	MusicGeneratorHandle generator = nullptr;

	FString TokenizerPath; 
	FString ModelPath;

	FTokenizerProxyPtr Tokenizer;

	//void* SearchStrategyData = nullptr;
	//TSearchStrategy SearchStrategy = nullptr;
	FOnSearch OnSearch;
	FOnGenerated OnGenerated;
	FOnInit OnInit;

	// OnGenerated mutex
	FCriticalSection Mutex;

	RunInstanceHandle runInstance = nullptr;
	BatchHandle batch = nullptr;
	AutoRegressiveBatchHandle Batch2;

	TArray<int32> EncodedTokens;
	//int32 LineNbMaxToken = 256;
	//int32 LineNbMaxToken = 512;
	int32 LineNbMaxToken = 1024;
	int32 NbMaxTokensAhead = 50;

	int32 NbBatchGen = 10;

	bool forceReupdate = false;

	FRunnableThread* Thread = nullptr;
	bool bShutdown = false;

	int32 NbTokensSinceLastRefresh = 0;

	////~Begin IAudioProxyDataFactory Interface.
	//virtual TSharedPtr<Audio::IProxyData> CreateProxyData(const Audio::FProxyDataInitParams& InitParams) override;
	////~ End IAudioProxyDataFactory Interface.

public:
	std::atomic_bool ShouldIgnoreNextToken = false;
	int32_t CacheTickToRemove = 0;
	float CacheMsToRemove = 0;

	FOnCacheRemoved OnCacheRemoved;

	FEvent* Semaphore = nullptr;

	std::atomic_int32_t CurrentTick;
	int32 NbMinTicksAhead = 200;
	int32 NbMaxTicksAhead = 400;

	bool ShouldResumeGeneration() const;
	bool ShouldSleep() const;
};