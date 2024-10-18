// Fill out your copyright notice in the Description page of Project Settings.


#include "MIDIGenerator.h"

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
		generator_generateNextToken(generator, input);
	}

	int32_t* outTokens = nullptr;
	int32_t outTokensSize = 0;

	input_decodeIDs(input, tok, &outTokens, &outTokensSize);

	tokens.Empty(outTokensSize);
	for (int32_t i = 0; i < outTokensSize; i++)
	{
		tokens.Push(outTokens[i]);
	}

	input_decodeIDs_free(outTokens);
}

bool UMIDIGenerator::RedirectorCall(int32 token)
{
	return redirector_call(redirector, token);
}

void UMIDIGenerator::Init(const FString& tokenizerPath, const FString& modelPath)
{
	env = createEnv(false);
	tok = createMidiTokenizer(TCHAR_TO_UTF8(*tokenizerPath));
	generator = createMusicGenerator();

	generator_loadOnnxModel(generator, env, TCHAR_TO_UTF8(*modelPath));

	int32 input_ids[] = {
	942,    65,  1579,  1842,   616,    46,  3032,  1507,   319,  1447,
	12384,  1016,  1877,   319, 15263,  3396,   302,  2667,  1807,  3388,
	2649,  1173,    50,   967,  1621,   256,  1564,   653,  1701,   377
	};

	int32 size = sizeof(input_ids) / sizeof(*input_ids);
	input = generator_generateInput(generator, input_ids, size);

	redirector = createRedirector();

	redirector_bindPitch(redirector, tok, "Pitch_", this, _OnPitch);
}

void UMIDIGenerator::Deinit()
{
	destroyRedirector(redirector);
	generator_generateInput_free(input);
	destroyMusicGenerator(generator);
	destroyMidiTokenizer(tok);
	destroyEnv(env);
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