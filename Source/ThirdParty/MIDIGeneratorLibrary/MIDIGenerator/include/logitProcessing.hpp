#pragma once

#include "logitProcessing.h"

API_EXPORT void stableSoftmaxRange(const SearchArgs& args, const RangeGroup& rangeGroup);
API_EXPORT void softmaxRange(const SearchArgs& args, const RangeGroup& rangeGroup);

API_EXPORT void specialPenaltyTransform(float* logits, const RangeGroup& rangeGroup, GenerationHistory& history, const SpecialPenaltyTransformArgs& args);

namespace Scales
{
    namespace Ionian
    {
        namespace CMajor
        {
            API_EXPORT const int32_t* get();
            API_EXPORT int32_t size();
        }

    }


}