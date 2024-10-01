#pragma once

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

using EnvHandle = Ort::Env*;
using MidiTokenizerHandle = MidiTokenizer*;
using MusicGeneratorHandle = MusicGenerator*;
using RedirectorHandle = Redirector*;

extern "C" 
{
    API_EXPORT EnvHandle createEnv(bool useLogging);
    API_EXPORT void destroyEnv(EnvHandle env);

    API_EXPORT MidiTokenizerHandle createMidiTokenizer(const char* tokenizerPath);
    API_EXPORT void destroyMidiTokenizer(MidiTokenizerHandle tokenizer);

    API_EXPORT MusicGeneratorHandle createMusicGenerator();
    API_EXPORT void destroyMusicGenerator(MusicGeneratorHandle musicGen);


    API_EXPORT RedirectorHandle createRedirector();
    API_EXPORT void destroyRedirector(RedirectorHandle handle);

    API_EXPORT void redirector_bindPitch(RedirectorHandle redirector, const MidiTokenizerHandle tokenizer, const char* prefix, void* data, void(*callback)(void*, unsigned char/*uint8*/ pitch));





}



