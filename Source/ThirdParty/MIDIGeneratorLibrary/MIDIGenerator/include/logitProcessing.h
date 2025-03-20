#pragma once

#include "fwd.h"
#include "range.h"

extern "C" 
{
    API_EXPORT int32_t greedySearch(const SearchArgs* args, RangeGroupHandle rangeGroup);

    API_EXPORT void stableSoftmaxRange(const SearchArgs* args, RangeGroupHandle rangeGroup);
    API_EXPORT void softmaxRange(const SearchArgs* args, RangeGroupHandle rangeGroup);

    API_EXPORT void stableSoftmax(float* logits, int32_t* indicesBegin, int32_t* indicesEnd);
    API_EXPORT void softmax(float* logits, int32_t* indicesBegin, int32_t* indicesEnd);

    API_EXPORT void sortLogits(float* logits, int32_t* indicesStart, int32_t* indicesEnd, int32_t nbLogitsToSort);

    // Stochastic Sampling: requires normalization (softmax) before
    API_EXPORT int32_t randomSampling(float* logits, int32_t* indicesStart, int32_t* indicesEnd);

    // logits should be sorted
    API_EXPORT int32_t topKSampling(float* logits, int32_t* indicesStart, int32_t* indicesEnd);
    API_EXPORT int32_t* topPSamplingFindCutoffIt(float* logits, int32_t* indicesStart, int32_t* indicesEnd, float cutoff);
    API_EXPORT int32_t topPSampling(float* logits, int32_t* indicesStart, int32_t* indicesEnd, float cutoff);

    API_EXPORT void temperatureTransform(float* logits, RangeGroupHandle rangeGroup, float temperature);
    
    API_EXPORT void customPenaltyTransform(float* logits, RangeGroupHandle rangeGroup, void* data, bool (*penaltyFunctor)(void* data, const int32_t token, float* outPenalty));
    API_EXPORT void repetitionPenaltyTransform(float* logits, RangeGroupHandle rangeGroup, float penalty, GenerationHistory* history, int32_t maxAge);

    typedef struct SpecialPenaltyTransformArgs
    {
        float pitchWindowSize = 100.f;
        float pitchMaxAdditivePenalty = 0.2f;
    } API_EXPORT SpecialPenaltyTransformArgs;

    API_EXPORT void specialPenaltyTransform(float* logits, RangeGroupHandle rangeGroup, GenerationHistory* history, const SpecialPenaltyTransformArgs* args);

    // API_EXPORT void pitchSetPenaltyTransform(float* logits, RangeGroupHandle rangeGroup);
    API_EXPORT void musicalScalePenaltyTransform(float* logits, RangeGroupHandle rangeGroup, const int32_t* pitches, int32_t nbPitches, float penaltyPerOutOfScalePitch, MidiTokenizerHandle tokenizer);

    // Adds penalty for tokens not in that range
    API_EXPORT void pitchRangePenaltyTransform(float* logits, RangeGroupHandle rangeGroup, const int32_t minPitch, const int32_t maxPitch, float penaltyPerOutOfRangePitch, MidiTokenizerHandle tokenizer);
    API_EXPORT void timeShiftRangePenaltyTransform(float* logits, RangeGroupHandle rangeGroup, const float minTimeShift, const float maxTimeShift, float penaltyPerOutOfRangeTimeShift, MidiTokenizerHandle tokenizer);

}

