#pragma once

#include <stdint.h>

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

#ifdef __cplusplus
    #define ASSERT_CPP_COMPILATION static_assert(true, "This code is being compiled as C++");
#else
    #define ASSERT_CPP_COMPILATION static_assert(false, "This code is not being compiled as C++");
#endif

#ifdef __cplusplus
namespace Ort
{
    struct Env;
    struct Session;
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
class RangeGroup;

class IPipeline;
class IAutoRegressivePipeline;

class AModel;
class ModelLoader;
class ModelLoadingParamsWrapper;
class GenerationHistory;

using EnvHandle = Ort::Env*;
using MidiTokenizerHandle = MidiTokenizer*;
using MusicGeneratorHandle = MusicGenerator*;
using RedirectorHandle = Redirector*;
using RunInstanceHandle = RunInstance*;
using TokenSequenceHandle = TokenSequence*;
using MidiConverterHandle = MIDIConverter*;
using BatchHandle = Batch*;
using RangeGroupHandle = RangeGroup*;

using IPipelineHandle = IPipeline*;
using AModelHandle = AModel*;
using ModelLoaderHandle = ModelLoader*;

using AutoRegressiveBatchHandle = int32_t;
using DataType = int32_t;

using TSearchStrategy = void (*)(const struct SearchArgs& args, void* searchStrategyData);

#else
typedef struct OrtEnvOpaque* EnvHandle;
typedef struct MidiTokenizerOpaque* MidiTokenizerHandle;
typedef struct MusicGeneratorOpaque* MusicGeneratorHandle;
typedef struct RedirectorOpaque* RedirectorHandle;
typedef struct RunInstanceOpaque* RunInstanceHandle;
typedef struct TokenSequenceOpaque* TokenSequenceHandle;
typedef struct MIDIConverterOpaque* MidiConverterHandle;
typedef struct BatchOpaque* BatchHandle;
typedef struct RangeGroupOpaque* RangeGroupHandle;

typedef struct IPipelineOpaque* IPipelineHandle;
typedef struct AModelOpaque* AModelHandle;
typedef struct ModelLoaderOpaque* ModelLoaderHandle;

typedef int32_t DataType;

typedef void (*TSearchStrategy)(const struct SearchArgs& args, void* searchStrategyData);
#endif
