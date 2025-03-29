#pragma once

#include <cstdio>
#include <string>
#include "utilities.hpp"
#include "fwd.hpp"

class IPipeline;

// A model can be loaded before the pipeline.
// That way, depending on metadata, we can decide automatically what pipeline to use.
class API_EXPORT AModel
{
public:
    virtual ~AModel() = default;

    virtual IPipeline* createPipeline() = 0;
};

class API_EXPORT AOnnxModel : public AModel
{
protected:
    UniquePtr<Ort::Session> session; // TODO : only put accessor instead?

public:
    CResult loadOnnxModel(const Ort::Env& env, const char* modelPath);
    void generate(const Ort::IoBinding& ioBindings, CppResult& outResult);

    virtual CResult onPostOnnxLoad() { return CResult(); }
    Ort::Session* getSession()
    {
        return &*session;
    }
};

// Inference Pipeline
class API_EXPORT IPipeline
{
public:
    virtual void preGenerate(CppResult& outResult) = 0;
    virtual void generate(CppResult& outResult) = 0;
    virtual void postGenerate(CppResult& outResult) = 0;

    virtual AModel* getModel() const = 0;

    virtual void reset() = 0;
};

using AutoRegressiveBatchHandle = int32_t;

// Inference Pipeline
class API_EXPORT IAutoRegressivePipeline : public IPipeline
{
public:
    // @TODO : per batch?
    virtual void setSearchStrategyData(void* searchStrategyData) = 0;
    virtual void setSearchStrategy(TSearchStrategy searchStrategy) = 0;

    virtual int32_t getNbBatches() const = 0;

    // returns the index of the new batch
    virtual AutoRegressiveBatchHandle addBatch() = 0;

    virtual void removeAllBatches() = 0;

    // If the model has to be updated, for example RNN state being reset if resetting the batch
    virtual int32_t batchGetLastGeneratedToken(AutoRegressiveBatchHandle batch) = 0;
    virtual void batchSet(AutoRegressiveBatchHandle batch, DataType* inputTokens, std::int32_t nbTokens, std::int32_t fromPos) = 0;

    virtual void setMaxInputLength(int32_t newMaxInputLength) = 0;
    
    // Owned by the pipeline, no need to destroy
    virtual void createHistory(const MidiTokenizer& tokenizer) = 0;
    virtual class GenerationHistory* getHistory(AutoRegressiveBatchHandle batchHandle) const = 0;
};
