#pragma once

#include <stdint.h>
#include "fwd.h"

typedef struct
{
    int32_t min;
    int32_t max;
} API_EXPORT Range;

static inline bool isColliding(const Range* a, const Range* b)
{
    return (a->min < b->max && a->max > b->min);
}

