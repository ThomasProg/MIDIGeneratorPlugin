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
private:
	MidiTokenizerHandle Tokenizer = nullptr;

public:
	FTokenizer() = default;
	FTokenizer(MidiTokenizerHandle InTokenizer)
		: Tokenizer(InTokenizer)
	{

	}

	bool IsPitch(int32 decodedToken) const
	{
		return isPitch(Tokenizer, decodedToken);
	}
	bool IsVelocity(int32 decodedToken) const
	{
		return isVelocity(Tokenizer, decodedToken);
	}
	bool IsDuration(int32 decodedToken) const
	{
		return isDuration(Tokenizer, decodedToken);
	}

	bool IsPosition(int32 decodedToken) const
	{
		return isPosition(Tokenizer, decodedToken);
	}

	bool IsBarNone(int32 decodedToken) const
	{
		return isBarNone(Tokenizer, decodedToken);
	}

	int32 GetNbDecodedTokens() const
	{
		return tokenizer_getNbDecodedTokens(Tokenizer);
	}
	int32 GetNbEncodedTokens() const
	{
		return tokenizer_getNbEncodedTokens(Tokenizer);
	}


	const char* DecodedTokenToString(int32 decodedToken) const
	{
		return tokenizer_decodedTokenToString(Tokenizer, decodedToken);
	}

	bool Load(const FString& TokenizerPath);

	bool IsValid() const
	{
		return Tokenizer == nullptr;
	}

	void LoadIfInvalid(const FString& TokenizerPath)
	{
		if (!IsValid())
		{
			Load(TokenizerPath);
		}
	}

	MidiTokenizerHandle GetTokenizer() const
	{
		return Tokenizer;
	}
};

/**
 * 
 */
UCLASS(BlueprintType, Category = "MIDI Generation", meta = (DisplayName = "MIDI Tokenizer"))
class MIDIGENERATORWRAPPER_API UTokenizerAsset : public UPrimaryDataAsset, public IAudioProxyDataFactory
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

		if (!Tokenizer.IsValid() || !Tokenizer->Tokenizer.IsValid() || (!Tokenizer->Tokenizer->IsValid()))
		{
			return nullptr;
		}
		else
		{
			return Tokenizer->Tokenizer->GetTokenizer();
		}
	}

	FTokenizerProxyPtr GetTokenizerPtr() const
	{
		return Tokenizer;
	}

	//~Begin IAudioProxyDataFactory Interface.
	virtual TSharedPtr<Audio::IProxyData> CreateProxyData(const Audio::FProxyDataInitParams& InitParams) override;
	//~ End IAudioProxyDataFactory Interface.
};
