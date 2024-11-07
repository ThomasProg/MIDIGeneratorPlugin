// Fill out your copyright notice in the Description page of Project Settings.


#include "MidiTokenizer.h"
//
//void _OnPitch(void* data, unsigned char pitch)
//{
//	if (data == nullptr)
//		return;
//
//	int32 pitchInt = int32(pitch);
//
//	//FMidiMsg& msg = * (FMidiMsg*) data;
//
//	//msg = FMidiMsg::CreateNoteOn(channel, note, velocity);
//
//
//	//if (GEngine)
//	//{
//	//	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, FString::Printf(TEXT("%d"), pitchInt));
//	//}
//
//	//((UMIDIGenerator*)data)->OnPitch(pitchInt);
//}

FMidiConverter::FMidiConverter()
{
	//redirector_bindPitch(redirector, tok, "Pitch_", this, _OnPitch);



}

FMidiConverter::~FMidiConverter()
{
}


void FMidiConverter::update()
{
    std::int32_t i = unplayedTokenIndex;

    struct Args
    {
        std::int32_t& i;
        std::int32_t& unplayedTokenIndex;
    };

    Args args{ i, unplayedTokenIndex };

    MidiConverterHandle converter = createMidiConverter();
    converterSetOnNote(converter, [](void* data, const Note& newNote)
        {
            Args& args = *(Args*)(data);
            args.unplayedTokenIndex = args.i + 1;



        });


    while (i < tokens.Num())
    {
        int j = i;
        converterProcessToken(converter, tokens.GetData(), tokens.Num(), &j, &args);
        //converter.processToken(tokens, i, &args);
        i++;
    }

    destroyMidiConverter(converter);
}