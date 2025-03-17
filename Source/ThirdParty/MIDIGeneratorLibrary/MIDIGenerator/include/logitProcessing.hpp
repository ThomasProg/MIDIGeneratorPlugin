#pragma once

#include "logitProcessing.h"

API_EXPORT void stableSoftmaxRange(const SearchArgs& args, const Range* ranges, size_t nbRanges);
API_EXPORT void softmaxRange(const SearchArgs& args, const Range* ranges, size_t nbRanges);

API_EXPORT void specialPenaltyTransform(float* logits, const Range* ranges, size_t nbRanges, GenerationHistory* history, const SpecialPenaltyTransformArgs& args);

namespace Scales
{
    namespace Ionian
    {
        namespace CMajor
        {
            API_EXPORT constexpr const int32_t* get();
            API_EXPORT constexpr int32_t size();
        }

    }


}