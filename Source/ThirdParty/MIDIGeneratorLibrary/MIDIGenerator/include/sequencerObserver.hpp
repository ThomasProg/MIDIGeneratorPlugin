#pragma once

#include "fwd.hpp"

class API_EXPORT ISequencerObserver
{
public:
    virtual void onCallback(int32_t hash, int32_t tick) = 0;
    virtual void onCallbackUndo(int32_t hash, int32_t tick) = 0;
};

