#pragma once

#include "mouse_button_codes.hpp"

namespace vkShade
{
    struct MouseMotionEvent
    {
        float x {0.0f};
        float y {0.0f};
    };

    struct MouseButtonEvent
    {
        MouseButton button;
        bool pressed {false};
    };
} // namespace vkShade
