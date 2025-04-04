// Copyright Prog'z. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "gen.h"
//#include "MIDIGenerator.generated.h"

//struct MIDIGENERATORWRAPPER_API FMIDIGenerator
//{
//	EnvHandle env;
//	MidiTokenizerHandle tok;
//	MusicGeneratorHandle generator;
//
//	void Init(const FString& tokenizerPath, const FString& modelPath);
//
//	void Deinit();
//
//	void Encode(const TArray<int32>& DecodedTokens, TArray<int32>& OutEncodedTokens);
//	void Decode(const TArray<int32>& EncodedTokens, TArray<int32>& OutDecodedTokens);
//};


///**
// * 
// */
//UCLASS(Blueprintable, BlueprintType)
//class MIDIGENERATORWRAPPER_API UMIDIGenerator : public UObject, public IAudioProxyDataFactory
//{
//	GENERATED_BODY()
//
//public:
//	FMIDIGenerator gen;
//
//	RunInstanceHandle runInstance;
//	BatchHandle batch;
//	RedirectorHandle redirector;
//
//
//
//	//virtual void PostInitProperties() override;
//	//virtual void BeginDestroy() override;
//
//	//UFUNCTION(BlueprintCallable)
//	//void LoadSoundfont(const FString& path = "C:/Users/thoma/PandorasBox/Projects/ModularMusicGenerationModules/Assets/Soundfonts/Touhou/Touhou.sf2");
//	//
//	//UFUNCTION(BlueprintCallable)
//	//void NoteOn(int32 channel, int32 key, int32 velocity);
//
//	//UFUNCTION(BlueprintCallable)
//	//void NoteOff(int32 channel, int32 key);
//
//public:
//	UFUNCTION(BlueprintCallable) 
//	void Init(const FString& tokenizerPath = "C:/Users/thoma/Documents/Unreal Projects/MIDITokCpp/tokenizer.json", const FString& modelPath = "C:/Users/thoma/Documents/Unreal Projects/MIDITokCpp/onnx_model_path/gpt2-midi-model3_past.onnx");
//	UFUNCTION(BlueprintCallable) 
//	void Deinit();
//
//	UFUNCTION(BlueprintCallable)
//	void Generate(int32 nbIterations, TArray<int32>& tokens);
//	UFUNCTION(BlueprintCallable)
//	bool RedirectorCall(int32 token);
//
//	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Callback")
//	void OnPitch(int32 pitch);
//};
