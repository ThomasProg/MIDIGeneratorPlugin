#pragma once

#include "fwd.h"
#include "note.h"

// Env
extern "C" 
{
    API_EXPORT EnvHandle createEnv(bool useLogging);
    API_EXPORT void destroyEnv(EnvHandle env);
}

// Generator
extern "C" 
{
    API_EXPORT MusicGeneratorHandle createMusicGenerator();
    API_EXPORT void destroyMusicGenerator(MusicGeneratorHandle musicGen);

    API_EXPORT void generator_loadOnnxModel(MusicGeneratorHandle generator, EnvHandle env, const char* path);
    API_EXPORT void generator_generateNextToken(MusicGeneratorHandle generator, RunInstanceHandle runInstance);

    API_EXPORT void generator_setConfig(MusicGeneratorHandle generator, int64_t num_attention_heads, int64_t hidden_size, int64_t num_layer);
}

// Redirector
extern "C" 
{
    API_EXPORT RedirectorHandle createRedirector();
    API_EXPORT void destroyRedirector(RedirectorHandle handle);
}

extern "C" 
{
    API_EXPORT TokenSequenceHandle createTokenSequence();
    API_EXPORT void destroyTokenSequence(TokenSequenceHandle handle);

    API_EXPORT void redirector_bindPitch(RedirectorHandle redirector, const MidiTokenizerHandle tokenizer, const char* prefix, void* data, void(*callback)(void*, std::uint8_t pitch));
    API_EXPORT bool redirector_call(RedirectorHandle redirector, std::int32_t token);

    API_EXPORT void tokenizer_decodeIDs(MidiTokenizerHandle tokenizer, const std::int32_t* inputIDs, std::int32_t size, std::int32_t** outputIDs, std::int32_t* outSize);
    API_EXPORT void tokenizer_decodeIDs_free(std::int32_t* outputIDs);
}


// Batch
extern "C" 
{
    API_EXPORT BatchHandle createBatch();
    API_EXPORT void destroyBatch(BatchHandle batch);

    API_EXPORT void batch_push(BatchHandle batch, DataType inInputId);
    API_EXPORT void batch_pop(BatchHandle batch);
    API_EXPORT void batch_set(BatchHandle batch, DataType* inputTokens, std::int32_t nbTokens, std::int32_t fromPos);

    API_EXPORT std::int32_t batch_size(BatchHandle batch);
    API_EXPORT void batch_getEncodedTokens(BatchHandle batch, DataType** outEncodedTokens, std::int32_t* outNbTokens);
}

// RunInstance
extern "C" 
{
    API_EXPORT RunInstanceHandle createRunInstance();
    API_EXPORT void destroyRunInstance(RunInstanceHandle runInstance);

    API_EXPORT void runInstance_addBatch(RunInstanceHandle runInstance, BatchHandle batch);
    API_EXPORT void runInstance_removeBatch(RunInstanceHandle runInstance, BatchHandle batch);
    API_EXPORT std::int32_t runInstance_nbBatches(RunInstanceHandle runInstance);

}

// Tokenizer
extern "C" 
{
    API_EXPORT MidiTokenizerHandle createMidiTokenizer(const char* tokenizerPath);
    API_EXPORT void destroyMidiTokenizer(MidiTokenizerHandle tokenizer);

    API_EXPORT bool isBarNone(MidiTokenizerHandle tokenizer, std::int32_t token);
    API_EXPORT bool isPosition(MidiTokenizerHandle tokenizer, std::int32_t token);
    API_EXPORT bool isPitch(MidiTokenizerHandle tokenizer, std::int32_t token);

    API_EXPORT std::int32_t getPosition(MidiTokenizerHandle tokenizer, std::int32_t token);
    API_EXPORT std::int32_t getPitch(MidiTokenizerHandle tokenizer, std::int32_t token);
}


// Converter
extern "C" 
{
    API_EXPORT MidiConverterHandle createMidiConverter();
    API_EXPORT void destroyMidiConverter(MidiConverterHandle converter);

    API_EXPORT void converterSetOnNote(MidiConverterHandle converter, void (*onNote)(void* data, const Note&));
    API_EXPORT void converterProcessToken(MidiConverterHandle converter, const int32_t* tokens, int32_t nbTokens, std::int32_t index, void* data);
    API_EXPORT void converterSetTokenizer(MidiConverterHandle converter, MidiTokenizerHandle tokenizer);




}



