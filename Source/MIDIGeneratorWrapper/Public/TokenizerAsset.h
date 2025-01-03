// Copyright Prog'z. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "gen.h"
#include "TokenizerAsset.generated.h"

struct FTokenizer;
class FTokenizerProxy;
using FTokenizerProxyPtr = TSharedPtr<FTokenizerProxy, ESPMode::ThreadSafe>;

class FTokenizerProxy final : public Audio::TProxyData<FTokenizerProxy>
{
public:
	IMPL_AUDIOPROXY_CLASS(FTokenizerProxy);

	MIDIGENERATORWRAPPER_API explicit FTokenizerProxy(MidiTokenizerHandle InTokenizer)
		: Tokenizer(MakeShared<FTokenizer>(InTokenizer))
	{

	}
	MIDIGENERATORWRAPPER_API explicit FTokenizerProxy(UTokenizerAsset* InTokenizer)
	{

	}
	MIDIGENERATORWRAPPER_API explicit FTokenizerProxy(const TSharedPtr<FTokenizer>& InTokenizer)
	{

	}

	TSharedPtr<FTokenizer> GetTokenizer()
	{
		return Tokenizer;
	}

	TSharedPtr<FTokenizer> Tokenizer = MakeShared<FTokenizer>();
};

struct MIDIGENERATORWRAPPER_API FTokenizer
{
	MidiTokenizerHandle Tokenizer = nullptr;
};

/**
 * 
 */
UCLASS(BlueprintType, Category = "MIDI Generation", meta = (DisplayName = "MIDI Tokenizer"))
class MIDIGENERATORWRAPPER_API UTokenizerAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
private:
	FTokenizerProxyPtr Tokenizer;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString TokenizerPath;

public:
	UFUNCTION(BlueprintCallable)
	void TryLoadTokenizer()
	{
		if (!TokenizerPath.IsEmpty())
		{
			MidiTokenizerHandle Tok = createMidiTokenizer(TCHAR_TO_UTF8(*TokenizerPath));
			if (Tok)
			{
				Tokenizer = MakeShared<FTokenizerProxy>(Tok);
			}
		}
	}

	UFUNCTION(BlueprintCallable)
	void TryLoadTokenizerFromPath(const FString& InTokenizerPath)
	{
		TokenizerPath = InTokenizerPath;
		TryLoadTokenizer();
	}

	MidiTokenizerHandle GetTokenizer()
	{
		if (Tokenizer == nullptr)
		{
			TryLoadTokenizer();
		}

		if (!Tokenizer.IsValid() || !Tokenizer->Tokenizer.IsValid() || Tokenizer->Tokenizer->Tokenizer == nullptr)
		{
			return nullptr;
		}
		else
		{
			return Tokenizer->Tokenizer->Tokenizer;
		}
	}
};
