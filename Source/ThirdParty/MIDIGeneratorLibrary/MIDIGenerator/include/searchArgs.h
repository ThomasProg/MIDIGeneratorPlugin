#pragma once

#include <stdint.h>
#include "fwd.h"

typedef struct SearchArgs
{
    float* logitsTensor;
    int32_t* outNextTokens;
    
    int32_t nbBatches;
    int32_t nbSequences;
    int32_t vocabSize;
} API_EXPORT SearchArgs;
