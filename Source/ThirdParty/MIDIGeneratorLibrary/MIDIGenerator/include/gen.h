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

namespace Ort
{
    struct Env;
}

class MidiTokenizer;
class MusicGenerator;
class Redirector;
struct Input;

using EnvHandle = Ort::Env*;
using MidiTokenizerHandle = MidiTokenizer*;
using MusicGeneratorHandle = MusicGenerator*;
using RedirectorHandle = Redirector*;
using InputHandle = Input*;

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

    API_EXPORT void redirector_bindPitch(RedirectorHandle redirector, const MidiTokenizerHandle tokenizer, const char* prefix, void* data, void(*callback)(void*, std::uint8_t pitch));
    API_EXPORT bool redirector_call(RedirectorHandle redirector, std::int32_t token);

    API_EXPORT InputHandle generator_generateInput(MusicGeneratorHandle generator, std::int32_t* inputIDs, std::int32_t size);
    API_EXPORT void generator_generateInput_free(InputHandle input);
    API_EXPORT void generator_generateNextToken(MusicGeneratorHandle generator, InputHandle input);

    API_EXPORT void input_decodeIDs(InputHandle input, MidiTokenizerHandle tokenizer, std::int32_t** outputIDs, std::int32_t* outSize);
    API_EXPORT void input_decodeIDs_free(std::int32_t* outputIDs);

    API_EXPORT void tokenizer_decodeIDs(MidiTokenizerHandle tokenizer, std::int32_t* inputIDs, std::int32_t size, std::int32_t** outputIDs, std::int32_t* outSize);
    API_EXPORT void tokenizer_decodeIDs_free(std::int32_t* outputIDs);
}



