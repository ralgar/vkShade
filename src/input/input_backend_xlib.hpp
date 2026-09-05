#pragma once

#include "input_manager.hpp"

typedef struct _XDisplay Display;
typedef unsigned long Window;

namespace vkShade
{
    class InputBackendXlib : public InputManager
    {
    public:
        InputBackendXlib(Display* display, Window window);
        ~InputBackendXlib();

        void process_events() override;

    private:
        Display* m_display;
        Display* m_wheelDisplay {nullptr};
        int      m_wheelXiOpcode {0};
        Window   m_window;
        Window   m_topLevelWindow {0};
        char     m_previousKeymap[32];  // Track previous keyboard state

        // Track previous mouse state
        glm::vec2 m_prevMousePos {0.0f, 0.0f};
        bool m_prevLeftButton = false;
        bool m_prevMiddleButton = false;
        bool m_prevRightButton = false;

        void handle_key_event(uint32_t keyCode, bool pressed);
        void query_mouse_state();
        void update_modifiers(unsigned int state);
        bool is_window_active(Display* display);
        bool is_pointer_inside_window(Display* display);
        static Window get_top_level_window(Display* display, Window window);
    };
} // namespace vkShade
