#pragma once

#include <cstdint>

struct Note
{
    std::int32_t tick;
    std::int32_t duration;
    std::int32_t pitch;
    std::int32_t velocity;
};

struct SearchArgs
{
    const float* logitsTensor;
    std::int32_t* outNextTokens;
    
    std::int32_t nbBatches;
    std::int32_t nbSequences;
    std::int32_t vocabSize;
};