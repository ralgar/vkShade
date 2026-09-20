#pragma once

#include <cstdint>

namespace vkShade
{
    constexpr uint64_t timestamp_tick_delta(uint64_t begin, uint64_t end,
                                            uint32_t validBits)
    {
        if (validBits == 0)
            return 0;

        if (validBits >= 64)
            return end - begin;

        const uint64_t mask = (uint64_t {1} << validBits) - 1;
        return (end - begin) & mask;
    }
} // namespace vkShade
