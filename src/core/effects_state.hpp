#pragma once

#include <atomic>

namespace vkShade
{
    struct EffectsState
    {
        std::atomic_bool enabled {true};
    };
} // namespace vkShade
