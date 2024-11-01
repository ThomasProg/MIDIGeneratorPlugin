#pragma once

#include <cstdint>

struct Note
{
    std::int32_t tick;
    std::int32_t duration;
    std::int32_t pitch;
    std::int32_t velocity;
};