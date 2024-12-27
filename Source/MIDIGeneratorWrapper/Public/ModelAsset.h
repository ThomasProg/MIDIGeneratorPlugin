// Copyright Prog'z. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ModelAsset.generated.h"

/**
 * 
 */
UCLASS(BlueprintType, Category = "MIDI Generation", meta = (DisplayName = "MIDI Model"))
class MIDIGENERATORWRAPPER_API UModelAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString ModelPath;
};
