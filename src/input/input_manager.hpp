#pragma once

#include <string>
#include <unordered_map>

#include <xkbcommon/xkbcommon.h>

#include "key_codes.hpp"

namespace vkShade
{
    class InputManager
    {
    public:
        InputManager();

        void bind_action(const std::string& actionName, vkShade::KeyCode key);

        void handle_keyboard_event(const xkb_keysym_t& keysym, bool pressed);

        bool is_action_pressed(const std::string& actionName) const;
        bool is_action_just_pressed(const std::string& actionName) const;
        bool is_action_just_released(const std::string& actionName) const;

        virtual void process_events() = 0;

        void update();

    private:
        struct ActionBinding
        {
            KeyCode keyCode;
            bool pressed = false;
            bool justPressed = false;
            bool justReleased = false;
        };

        std::unordered_map<std::string, ActionBinding> m_actionBindings;

        std::unordered_map<KeyCode, bool> m_currentKeyStates;
        std::unordered_map<KeyCode, bool> m_previousKeyStates;

        KeyCode map_key(xkb_keysym_t keysym);
    };
} // namespace vkShade
