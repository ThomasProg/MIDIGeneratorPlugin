#pragma once

#include <cstdint>

#if defined(_WIN32) || defined(_WIN64)
    #ifdef BUILD_STATIC
        #define API_EXPORT 
    #else
        #ifdef BUILD_DLL
            #define API_EXPORT __declspec(dllexport)
        #else
            #define API_EXPORT __declspec(dllimport)
        #endif
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
class Redirector;
struct RunInstance;
class TokSequence;
using TokenSequence = TokSequence;
class MIDIConverter;
struct Batch;
struct SearchArgs;

using EnvHandle = Ort::Env*;
using MidiTokenizerHandle = MidiTokenizer*;
using MusicGeneratorHandle = MusicGenerator*;
using RedirectorHandle = Redirector*;
using RunInstanceHandle = RunInstance*;
using TokenSequenceHandle = TokenSequence*;
using MidiConverterHandle = MIDIConverter*;
using BatchHandle = Batch*;

using DataType = std::int32_t;

using TSearchStrategy = void (*)(const struct SearchArgs& args, void* searchStrategyData);