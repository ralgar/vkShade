#pragma once

#include "input_manager.hpp"

typedef struct xcb_connection_t xcb_connection_t;
typedef uint32_t xcb_window_t;

namespace vkShade
{
    class InputManagerXcb : public InputManager
    {
    public:
        InputManagerXcb(xcb_connection_t* connection, xcb_window_t window);

        void process_events() override;

    private:
        xcb_connection_t* m_connection {nullptr};
        xcb_window_t      m_window     {0};

        void handle_key_event(uint32_t keyCode, bool pressed);
        void on_mouse_button(uint8_t button, bool pressed);
        void on_mouse_motion(int16_t x, int16_t y);
        void update_modifiers(uint16_t state);
    };
} // namespace vkShade
