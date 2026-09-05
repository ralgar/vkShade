#pragma once

#include <span>

#include <glm/vec2.hpp>

namespace vkShade
{
    glm::vec2 decode_xinput_relative_motion(
        std::span<const unsigned char> valuatorMask,
        const double* values);

    bool should_forward_xinput_wheel(
        int button,
        bool sourceEvent,
        bool pressed,
        bool rawFallbackActive,
        bool wheelObserverActive);
} // namespace vkShade
