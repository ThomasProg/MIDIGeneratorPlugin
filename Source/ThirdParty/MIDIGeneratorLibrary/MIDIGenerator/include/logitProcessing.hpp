#pragma once

#include "logitProcessing.h"

API_EXPORT void stableSoftmaxRange(const SearchArgs& args, const RangeGroup& rangeGroup);
API_EXPORT void softmaxRange(const SearchArgs& args, const RangeGroup& rangeGroup);

API_EXPORT void specialPenaltyTransform(float* logits, const RangeGroup& rangeGroup, GenerationHistory& history, const SpecialPenaltyTransformArgs& args);

namespace Scales
{
    namespace Ionian
    {
        namespace Major
        {
            API_EXPORT const int32_t* get();
            API_EXPORT int32_t size();
        }

    }

    namespace Mixolydian
    {
        API_EXPORT const int32_t* get();
        API_EXPORT int32_t size();
    }

    namespace Melodic
    {
        namespace Minor
        {
            API_EXPORT const int32_t* get();
            API_EXPORT int32_t size();
        }

    }

    namespace Harmonic
    {
        namespace Minor
        {
            API_EXPORT const int32_t* get();
            API_EXPORT int32_t size();
        }

    }

    namespace WholeTone
    {
        API_EXPORT const int32_t* get();
        API_EXPORT int32_t size();
    }

    namespace Blues
    {
        API_EXPORT const int32_t* get();
        API_EXPORT int32_t size();
    }

    namespace Pentatonic
    {
        namespace Major
        {
            API_EXPORT const int32_t* get();
            API_EXPORT int32_t size();
        }

    }

    namespace Pentatonic
    {
        namespace Minor
        {
            API_EXPORT const int32_t* get();
            API_EXPORT int32_t size();
        }

    }

    namespace Hungarian
    {
        namespace Minor
        {
            API_EXPORT const int32_t* get();
            API_EXPORT int32_t size();
        }

    }

    namespace Byzantine
    {
        API_EXPORT const int32_t* get();
        API_EXPORT int32_t size();
    }

    namespace Diminished
    {
        API_EXPORT const int32_t* get();
        API_EXPORT int32_t size();
    }

}