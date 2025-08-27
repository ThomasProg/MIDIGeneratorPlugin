#pragma once

#include <cstdint>
#include <cassert>
#include <iostream>

#include "logitProcessing.hpp"
#include "fwd.h"

class LogitsNode
{
public:
    MidiTokenizerHandle tokenizer = nullptr;
    RangeGroupHandle rangeGroup = nullptr;
    float* logitsTensor = nullptr;

    LogitsNode(MidiTokenizerHandle inTokenizer, RangeGroupHandle inRangeGroup, float* inLogitsTensor)
        : tokenizer(inTokenizer), rangeGroup(inRangeGroup), logitsTensor(inLogitsTensor)
    {

    }
};

template<typename TCurrent = void, typename TPrevChain = void>
class Chain
{
public:
    TCurrent current;
    TPrevChain prev;

    Chain(TCurrent inCurrent, TPrevChain inPrev) : current(inCurrent), prev(inPrev)
    {

    }
};

template<>
class Chain<void, void>
{
public:
    Chain() = default;
};

using Penalty = float;

class API_EXPORT PolymorphicPenaltyTransform
{
    virtual LogitsNode& operator()(LogitsNode& source) = 0;
};

template<typename T>
class TPolymorphicPenaltyTransform : public PolymorphicPenaltyTransform
{
private:
    T transform;

public:
    virtual LogitsNode& operator()(LogitsNode& source) override
    {
        transform(source);
    }
};

class API_EXPORT PenaltyTransform
{
protected:
    float penalty;

    PenaltyTransform(Penalty inPenalty)
        : penalty(inPenalty)
    {

    }
};

class TestTransform
{
public:
    std::int32_t testIndex;

public:
    TestTransform(std::int32_t inIndex) : testIndex(inIndex) {}

    LogitsNode& operator()(LogitsNode& source)
    {
        std::cout << "Test: " << testIndex << std::endl;
        return source;
    }
};

class API_EXPORT MusicScalePenaltyTransform : public PenaltyTransform
{
private:
    const std::int32_t* scale;
    std::int32_t scaleSize;

public:
    MusicScalePenaltyTransform(const std::int32_t* inScale, std::int32_t inScaleSize, Penalty inPenalty)
        : PenaltyTransform(inPenalty), scale(inScale), scaleSize(inScaleSize)
    {
        // assert(scale != nullptr && scaleSize != 0);
    }

    LogitsNode& operator()(LogitsNode& source)
    {
        musicalScalePenaltyTransform(source.logitsTensor, source.rangeGroup, scale, scaleSize, penalty, source.tokenizer);
        return source;
    }
};

class PitchRangePenaltyTransform : public PenaltyTransform
{
private:
    const std::int32_t* scale;
    std::int32_t scaleSize;

public:
    PitchRangePenaltyTransform(const std::int32_t* inScale, std::int32_t inScaleSize, Penalty inPenalty)
        : PenaltyTransform(inPenalty), scale(inScale), scaleSize(inScaleSize)
    {
        // assert(scale != nullptr && scaleSize != 0);
    }

    LogitsNode& operator()(LogitsNode& source)
    {
        // pitchRangePenaltyTransform(source.logitsTensor, source.rangeGroup, scale, scaleSize, penalty, source.tokenizer);
        return source;
    }
};

// class MusicScalePenaltyTransform
// {
// public:
//     LogitsNode& operator()(LogitsNode& source)
//     {
//         return source;
//     }
// };


template<typename T>
LogitsNode& operator>>(LogitsNode& source, T&& transform)
{
    return transform(source);
}

template<typename TPrev, typename TPrevPrev>
LogitsNode& operator>>(LogitsNode& source, Chain<TPrev, TPrevPrev>& transform)
{
    return source >> transform.prev >> transform.current;
}

LogitsNode& operator>>(LogitsNode& source, Chain<void, void>& transform)
{
    return source;
}

template<typename TPrev, typename TPrevPrev, typename T>
Chain<T, Chain<TPrev, TPrevPrev>> operator>>(Chain<TPrev, TPrevPrev>& source, T&& transform)
{
    Chain<T, Chain<TPrev, TPrevPrev>> ret(std::forward<T>(transform), source);
    return ret;
}

template<typename T, typename F>
void visit(T&& chain, F&& functor)
{
    visit(chain.prev, functor);
    functor(chain.current);
}

template<typename F>
void visit(Chain<void, void> chain, F&& functor)
{

}
