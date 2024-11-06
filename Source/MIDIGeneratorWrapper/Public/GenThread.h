// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MIDIGenerator.h"

DECLARE_DELEGATE(FOnGenerated);

/**
 * 
 */
class MIDIGENERATORWRAPPER_API FGenThread : public FRunnable
{
public:
	//FGenThread(const FString& TokenizerPath, const FString& ModelPath);
	void Start(const FString& TokenizerPath, const FString& ModelPath, const TArray<int32>& InTokens);
	~FGenThread();


	FString TokenizerPath; 
	FString ModelPath;
	FMIDIGenerator Generator;

	//EnvHandle env;
	//MidiTokenizerHandle tok;
	//MusicGeneratorHandle generator;

	//TWeakObjectPtr<UMIDIGenerator> Generator = nullptr;

	RunInstanceHandle runInstance;
	BatchHandle batch;

	TArray<int32> EncodedLine;
	//TArray<int32> DecodedLine;
	int32 LineNbMaxToken = 256;
	int32 NbMaxTokensAhead = 50;

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
};