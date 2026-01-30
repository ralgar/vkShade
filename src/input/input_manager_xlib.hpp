#pragma once

#include "input_manager.hpp"

typedef struct _XDisplay Display;

namespace vkShade
{
    class InputManagerXlib : public InputManager
    {
    public:
        InputManagerXlib(Display* display);
        ~InputManagerXlib();

        void process_events() override;

    private:
        Display* m_display;
        char     m_previousKeymap[32];  // Track previous keyboard state

        void handle_key_event(uint32_t keyCode, bool pressed);
        void update_modifiers(unsigned int state);
    };
} // namespace vkShade
