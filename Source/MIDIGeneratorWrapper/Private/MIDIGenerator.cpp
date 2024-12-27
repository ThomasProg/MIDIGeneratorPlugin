// Copyright Prog'z. All Rights Reserved.


#include "MIDIGenerator.h"

void FMIDIGenerator::Init(const FString& tokenizerPath, const FString& modelPath)
{
	env = createEnv(false);
	tok = createMidiTokenizer(TCHAR_TO_UTF8(*tokenizerPath));
	generator = createMusicGenerator();

	generator_loadOnnxModel(generator, env, TCHAR_TO_UTF8(*modelPath));
	generator_setConfig(generator, 4, 256, 6);
}

void FMIDIGenerator::Deinit()
{
	destroyMusicGenerator(generator);
	destroyMidiTokenizer(tok);
	destroyEnv(env);
}

void FMIDIGenerator::Encode(const TArray<int32>& DecodedTokens, TArray<int32>& OutEncodedTokens)
{


}
void FMIDIGenerator::Decode(const TArray<int32>& EncodedTokens, TArray<int32>& OutDecodedTokens)
{
	int32_t* outTokens = nullptr;
	int32_t outTokensSize = 0;
	tokenizer_decodeIDs(tok, EncodedTokens.GetData(), EncodedTokens.Num(), &outTokens, &outTokensSize);

	OutDecodedTokens.SetNumUninitialized(outTokensSize);
	for (int32 i = 0; i < outTokensSize; i++)
	{
		OutDecodedTokens[i] = outTokens[i];
	}
}



void _OnPitch(void* data, unsigned char pitch)
{
	int32 pitchInt = int32(pitch);

	//if (GEngine)
	//{
	//	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, FString::Printf(TEXT("%d"), pitchInt));
	//}

	((UMIDIGenerator*) data)->OnPitch(pitchInt);
}

void UMIDIGenerator::Generate(int32 nbIterations, TArray<int32>& tokens)
{
	for (int i = 0; i < nbIterations; i++)
	{
		generator_generateNextToken(gen.generator, runInstance);
	}


	int32_t* encodedTokens = nullptr;
	std::int32_t nbEncodedTokens;
	batch_getEncodedTokens(batch, &encodedTokens, &nbEncodedTokens);

	int32_t* decodedTokens = nullptr;
	int32_t nbDecodedTokens = 0;
	tokenizer_decodeIDs(gen.tok, encodedTokens, nbEncodedTokens, &decodedTokens, &nbDecodedTokens);

	tokens.Empty(nbDecodedTokens);
	for (int32_t i = 0; i < nbDecodedTokens; i++)
	{
		tokens.Push(decodedTokens[i]);
	}

	tokenizer_decodeIDs_free(decodedTokens);
}

bool UMIDIGenerator::RedirectorCall(int32 token)
{
	return redirector_call(redirector, token);
}

void UMIDIGenerator::Init(const FString& tokenizerPath, const FString& modelPath)
{
	gen.Init(tokenizerPath, modelPath);

	int32 input_ids[] = {
	942,    65,  1579,  1842,   616,    46,  3032,  1507,   319,  1447,
	12384,  1016,  1877,   319, 15263,  3396,   302,  2667,  1807,  3388,
	2649,  1173,    50,   967,  1621,   256,  1564,   653,  1701,   377
	};

	int32 size = sizeof(input_ids) / sizeof(*input_ids);
	runInstance = createRunInstance();
	batch = createBatch();
	runInstance_addBatch(runInstance, batch);
	batch_set(batch, input_ids, size, 0);

	redirector = createRedirector();

	redirector_bindPitch(redirector, gen.tok, "Pitch_", this, _OnPitch);
}

void UMIDIGenerator::Deinit()
{
	destroyRedirector(redirector);
	destroyBatch(batch);
	destroyRunInstance(runInstance);

	gen.Deinit();
}

void UMIDIGenerator::OnPitch_Implementation(int32 pitch)
{

}

//void UFluidsynthAudioPlayer::PostInitProperties()
//{
//	UObject::PostInitProperties();
//
//	if (!HasAnyFlags(RF_ClassDefaultObject))
//	{
//		// Create settings and synth
//		settings = new_fluid_settings();
//		synth = new_fluid_synth(settings);
//
//		// Create an audio driver
//		//driverCallback = new_fluid_audio_driver2(settings, fluidsynth_callback, this);
//		driverCallback = new_fluid_audio_driver(settings, synth);
//	}
//}
//
//void UFluidsynthAudioPlayer::BeginDestroy()
//{
//	if (driverCallback)
//	{
//		delete_fluid_audio_driver(driverCallback);
//		driverCallback = nullptr;
//	}
//
//	if (synth)
//	{
//		delete_fluid_synth(synth);
//		synth = nullptr;
//	}
//	if (settings)
//	{
//		delete_fluid_settings(settings);
//		settings = nullptr;
//	}
//
//	UObject::BeginDestroy();
//}

