#pragma once

#include <cstdint>

#if defined(_WIN32) || defined(_WIN64)
    #ifdef BUILD_DLL
        #define API_EXPORT __declspec(dllexport)
    #else
        #define API_EXPORT __declspec(dllimport)
    #endif
#else
    #define API_EXPORT __attribute__((visibility("default")))
#endif

#include "fwd.h"
#include "note.h"

extern "C" 
{
    API_EXPORT EnvHandle createEnv(bool useLogging);
    API_EXPORT void destroyEnv(EnvHandle env);

    API_EXPORT MidiTokenizerHandle createMidiTokenizer(const char* tokenizerPath);
    API_EXPORT void destroyMidiTokenizer(MidiTokenizerHandle tokenizer);

    API_EXPORT MusicGeneratorHandle createMusicGenerator();
    API_EXPORT void destroyMusicGenerator(MusicGeneratorHandle musicGen);

    API_EXPORT void generator_loadOnnxModel(MusicGeneratorHandle generator, EnvHandle env, const char* path);

    API_EXPORT RedirectorHandle createRedirector();
    API_EXPORT void destroyRedirector(RedirectorHandle handle);

    API_EXPORT TokenSequenceHandle createTokenSequence();
    API_EXPORT void destroyTokenSequence(TokenSequenceHandle handle);

    API_EXPORT void redirector_bindPitch(RedirectorHandle redirector, const MidiTokenizerHandle tokenizer, const char* prefix, void* data, void(*callback)(void*, std::uint8_t pitch));
    API_EXPORT bool redirector_call(RedirectorHandle redirector, std::int32_t token);

    API_EXPORT InputHandle generator_generateInput(MusicGeneratorHandle generator, std::int32_t* inputIDs, std::int32_t size);
    API_EXPORT void generator_generateInput_free(InputHandle input);
    API_EXPORT void generator_generateNextToken(MusicGeneratorHandle generator, InputHandle input);

    API_EXPORT void input_decodeIDs(InputHandle input, MidiTokenizerHandle tokenizer, std::int32_t** outputIDs, std::int32_t* outSize);
    API_EXPORT void input_decodeIDs_free(std::int32_t* outputIDs);

    API_EXPORT void tokenizer_decodeIDs(MidiTokenizerHandle tokenizer, std::int32_t* inputIDs, std::int32_t size, std::int32_t** outputIDs, std::int32_t* outSize);
    API_EXPORT void tokenizer_decodeIDs_free(std::int32_t* outputIDs);



    API_EXPORT bool isBarNone(MidiTokenizerHandle tokenizer, std::int32_t token);
    API_EXPORT bool isPosition(MidiTokenizerHandle tokenizer, std::int32_t token);
    API_EXPORT bool isPitch(MidiTokenizerHandle tokenizer, std::int32_t token);

    API_EXPORT std::int32_t getPosition(MidiTokenizerHandle tokenizer, std::int32_t token);
    API_EXPORT std::int32_t getPitch(MidiTokenizerHandle tokenizer, std::int32_t token);


    API_EXPORT MidiConverterHandle createMidiConverter();
    API_EXPORT void destroyMidiConverter(MidiConverterHandle converter);

    API_EXPORT void converterSetOnNote(MidiConverterHandle converter, void (*onNote)(void* data, const Note&));
    API_EXPORT void converterProcessToken(MidiConverterHandle converter, const int32_t* tokens, int32_t nbTokens, std::int32_t index, void* data);
    API_EXPORT void converterSetTokenizer(MidiConverterHandle converter, MidiTokenizerHandle tokenizer);




}



