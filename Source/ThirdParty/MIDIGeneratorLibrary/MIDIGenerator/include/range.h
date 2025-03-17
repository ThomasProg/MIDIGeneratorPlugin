#pragma once

#include <stdint.h>
#include "fwd.h"

typedef struct
{
    int32_t min;
    int32_t max;
} API_EXPORT Range;

extern "C"
{
API_EXPORT bool isColliding(const Range* a, const Range* b);
API_EXPORT bool rangeSize(const Range* a);
API_EXPORT uint64_t rangeGroupSize(const RangeGroupHandle rangeGroup);
API_EXPORT void rangeGroupWrite(const RangeGroupHandle rangeGroup, int32_t* buffer);
}