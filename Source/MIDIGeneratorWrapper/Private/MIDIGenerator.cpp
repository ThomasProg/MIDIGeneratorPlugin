// Fill out your copyright notice in the Description page of Project Settings.


#include "MIDIGenerator.h"

#include "gen.h"

void OnPitch(void* data, unsigned char pitch)
{
	int pitchInt = int(pitch);

	//if (GEngine)
	//{
	//	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, FString::Printf(TEXT("%d"), pitchInt));
	//}

}

void UMIDIGenerator::Generate()
{
	EnvHandle env = createEnv(false);
	MidiTokenizerHandle tok = createMidiTokenizer("C:/Users/thoma/Documents/Unreal Projects/MIDITokCpp/tokenizer.json");
	MusicGeneratorHandle generator = createMusicGenerator();
	RedirectorHandle redirector = createRedirector();

	redirector_bindPitch(redirector, tok, "Pitch_", this, OnPitch);


	destroyRedirector(redirector);
	destroyMusicGenerator(generator);
	destroyMidiTokenizer(tok);
	destroyEnv(env);
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