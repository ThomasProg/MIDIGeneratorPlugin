#pragma once

#include "fwd.h"
#include "note.h"

extern "C" 
{
    API_EXPORT MidiConverterHandle createConverterFromTokenizer(const MidiTokenizer* tokenizer);
    
    API_EXPORT MidiConverterHandle createREMIConverter();
    API_EXPORT MidiConverterHandle createTSDConverter();
    API_EXPORT void destroyMidiConverter(MidiConverterHandle converter);

    API_EXPORT void converterSetOnNote(MidiConverterHandle converter, void (*onNote)(void* data, const Note&));
    // Index increases depending on the amount of tokens "converted"
    API_EXPORT bool converterProcessToken(MidiConverterHandle converter, const int32_t* tokens, int32_t nbTokens, int32_t* index, void* data);
    API_EXPORT void converterSetTokenizer(MidiConverterHandle converter, const MidiTokenizer* tokenizer);




}
