#pragma once

#include <stdint.h>
#include "fwd.h"

typedef struct Note
{
    int32_t tick;
    int32_t duration;
    int32_t pitch;
    int32_t velocity;
} API_EXPORT Note;

typedef struct SearchArgs
{
    float* logitsTensor;
    int32_t* outNextTokens;
    
    int32_t nbBatches;
    int32_t nbSequences;
    int32_t vocabSize;
} API_EXPORT SearchArgs;

