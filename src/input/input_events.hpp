#pragma once

#include <string>

#include "key_codes.hpp"
#include "mouse_button_codes.hpp"

namespace vkShade
{
    struct KeyboardEvent
    {
        KeyCode keyCode;
        bool pressed;
    };

    struct TextInputEvent
    {
        std::string text;
    };

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

    struct MouseWheelEvent
    {
        float x {0.0f};
        float y {0.0f};
    };
} // namespace vkShade
