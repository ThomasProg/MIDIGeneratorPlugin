// Fill out your copyright notice in the Description page of Project Settings.


#include "GenThread.h"

//FGenThread::FGenThread(const FString& TokenizerPath, const FString& ModelPath)
void FGenThread::Start(const FString& InTokenizerPath, const FString& InModelPath, const TArray<int32>& InTokens)
{
	this->TokenizerPath = InTokenizerPath;
	this->ModelPath = InModelPath;
	EncodedLine = InTokens;

	Thread = FRunnableThread::Create(this, TEXT("GenThread"));
}

FGenThread::~FGenThread()
{
	if (Thread)
	{
		// Kill() is a blocking call, it waits for the thread to finish.
		// Hopefully that doesn't take too long
		Thread->Kill();
		delete Thread;
	}
}

bool FGenThread::Init() 
{
	Generator.Init(TokenizerPath, ModelPath);
	runInstance = createRunInstance();
	batch = createBatch();
	runInstance_addBatch(runInstance, batch);
	batch_set(batch, EncodedLine.GetData(), EncodedLine.Num(), 0);


	/* Should the thread start? */
	return true;
}

void FGenThread::AddTokenGroupToInsert(const TArray<int32>& TokenGroup)
{
	TokenGroupToInsert = TokenGroup;
}

void FGenThread::TryInsertTokenGroup()
{
	//TArray<int32> DecodedLine = Decode(EncodedLine);

	//int32 indexAfterTick = FindIndexAfterTick(DecodedLine, TokenGroupInsertTick);
	//if (indexAfterTick < DecodedLine.Num())
	//{
	//	DecodedLine.SetNum(indexAfterTick, EAllowShrinking::No);

	//	for (int32 token : TokenGroupToInsert)
	//	{
	//		DecodedLine.Push(token);
	//	}
	//}

	//EncodedLine = Encode(DecodedLine);
}

uint32 FGenThread::Run() 
{
	{
		TArray<int32> Context;
		int32 start = FMath::Max(0, EncodedLine.Num() - LineNbMaxToken);
		for (int32 i = start; i < EncodedLine.Num(); i++)
		{
			Context.Add(EncodedLine[i]);
		}

		batch_set(batch, Context.GetData(), Context.Num(), start);
	}

	ensureMsgf(NbBatchGen < LineNbMaxToken, TEXT("Reduce NbBatchGen!"));

	while (!bShutdown) 
	{
		/* Work on a dedicated thread */
		//Generator->Generate(10, Tokens);

		for (int32 i = 0; i < NbBatchGen; i++)
		{
			generator_generateNextToken(Generator.generator, runInstance);
		}

		int32* tokens;
		int32 tokensSize;
		batch_getEncodedTokens(batch, &tokens, &tokensSize);

		//int32 newToken = generateNextToken(Context);
		//int32 newToken = tokens[tokensSize - 1];
		//EncodedLine.Add(newToken);


		//TryInsertTokenGroup();






		//Context.Add(newToken);

		//if (Context.Num() > LineNbMaxToken)
		//{
		//	Context.RemoveAt(0, EAllowShrinking::No);
		//}

		//// while generated ahead enough
		//while (EncodedLine.Num() - NextTokenIndexToPlay > NbMaxTokensAhead)
		//{
		//	FPlatformProcess::Sleep(0.1f);
		//}

		if (!bShutdown)
			OnGenerated.ExecuteIfBound(tokens, tokensSize, NbBatchGen);
	}

	return 0;
}

void FGenThread::Exit() 
{
	runInstance_removeBatch(runInstance, batch);
	destroyBatch(batch);
	destroyRunInstance(runInstance);

	Generator.Deinit();
}

void FGenThread::Stop() 
{
	bShutdown = true;
}