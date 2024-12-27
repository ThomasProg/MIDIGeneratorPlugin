// Copyright Prog'z. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "TokenizerAsset.generated.h"

/**
 * 
 */
UCLASS(BlueprintType, Category = "MIDI Generation", meta = (DisplayName = "MIDI Tokenizer"))
class MIDIGENERATORWRAPPER_API UTokenizerAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString TokenizerPath;
};
