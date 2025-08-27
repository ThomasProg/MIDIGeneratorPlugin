#pragma once

#include "fwd.h"

// The generated logits for a single batch
typedef struct LogitsView
{
    float* logits;
    int32_t vocabSize;
} API_EXPORT LogitsView;

class API_EXPORT AutoRegressivePipelineObserver
{
public:
    virtual void OnGenerationStarted() {}
    virtual void OnGenerationFinished() {}

    virtual void OnLogitsGenerated(const LogitsView& logitsView) {}
    virtual void OnTokenSelected(int32_t newToken) {}
    virtual void OnTokenAdded() {}

    virtual void OnNoteGenerated(const Note& note) {}
};
