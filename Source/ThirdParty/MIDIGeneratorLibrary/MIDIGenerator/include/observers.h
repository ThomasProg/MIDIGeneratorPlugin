#pragma once

#include "fwd.h"

// Env
extern "C" 
{
    API_EXPORT class AutoRegressivePipelineObserver* createMixerObserver();
    API_EXPORT void destroyObserver(AutoRegressivePipelineObserver* observer);
}
