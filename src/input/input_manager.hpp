#pragma once

#include <string>
#include <unordered_map>

#include "key_codes.hpp"

typedef struct xkb_context xkb_context;
typedef struct xkb_keymap xkb_keymap;
typedef struct xkb_state xkb_state;
typedef uint32_t xkb_keysym_t;

namespace vkShade
{
    class InputManager
    {
    public:
        virtual ~InputManager();

        void bind_action(const std::string& actionName, vkShade::KeyCode key);

        void handle_keyboard_event(const xkb_keysym_t& keysym, bool pressed);

        bool is_action_pressed(const std::string& actionName) const;
        bool is_action_just_pressed(const std::string& actionName) const;
        bool is_action_just_released(const std::string& actionName) const;

        virtual void process_events() = 0;

        void update();

    protected:
        InputManager();  // Prevent direct instantiation

        xkb_context* m_xkbContext {nullptr};
        xkb_keymap*  m_xkbKeymap  {nullptr};
        xkb_state*   m_xkbState   {nullptr};

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
