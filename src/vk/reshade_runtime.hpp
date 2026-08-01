#pragma once

#include <chrono>

namespace vkShade
{
    inline float reshade_frame_time(std::chrono::steady_clock::duration duration)
    {
        return std::chrono::duration<float, std::milli>(duration).count();
    }
}
