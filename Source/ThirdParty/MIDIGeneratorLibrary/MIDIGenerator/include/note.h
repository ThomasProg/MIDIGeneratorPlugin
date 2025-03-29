#pragma once

#include <stdint.h>
#include "fwd.h"

typedef struct Note
{
    int32_t tick;
    int32_t duration;
    int32_t pitch;
    int32_t velocity;
} API_EXPORT Note;

