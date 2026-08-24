#pragma once

#include <string>
#include <unordered_map>

#include <glm/vec2.hpp>

#include "key_codes.hpp"
#include "mouse_button_codes.hpp"

typedef struct xkb_context xkb_context;
typedef struct xkb_keymap xkb_keymap;
typedef struct xkb_state xkb_state;
typedef uint32_t xkb_keysym_t;

namespace vkShade
{
    constexpr KeyCode DEFAULT_KEY_EFFECTS_TOGGLE = KeyCode::KEY_INSERT;
    constexpr KeyCode DEFAULT_KEY_GUI_TOGGLE = KeyCode::KEY_HOME;

    class InputManager
    {
    public:
        virtual ~InputManager();

        void bind_action(const std::string& actionName, vkShade::KeyCode key);

        bool is_action_pressed(const std::string& actionName) const;
        bool is_action_just_pressed(const std::string& actionName) const;
        bool is_action_just_released(const std::string& actionName) const;

        // Mouse state
        bool is_mouse_button_pressed(MouseButton button) const;
        bool is_mouse_button_just_pressed(MouseButton button) const;
        bool is_mouse_button_just_released(MouseButton button) const;

        glm::vec2 mouse_position() const { return m_currentMousePosition; }
        glm::vec2 mouse_delta() const { return m_mouseDelta; }

        virtual void process_events() = 0;

        void update();

    protected:
        InputManager();  // Prevent direct instantiation

        xkb_context* m_xkbContext {nullptr};
        xkb_keymap*  m_xkbKeymap  {nullptr};
        xkb_state*   m_xkbState   {nullptr};

        void handle_keyboard_event(const xkb_keysym_t& keysym, bool pressed);
        void handle_mouse_button_event(MouseButton button, bool pressed);
        void handle_mouse_motion_event(float x, float y);

        void on_keybind_changed(const std::string& configKey, std::string enumString);

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

        glm::vec2 m_currentMousePosition {0.0f, 0.0f};
        glm::vec2 m_previousMousePosition {0.0f, 0.0f};
        glm::vec2 m_mouseDelta {0.0f, 0.0f};

        std::unordered_map<MouseButton, bool> m_currentMouseStates;
        std::unordered_map<MouseButton, bool> m_previousMouseStates;

        KeyCode map_key(xkb_keysym_t keysym);
    };
} // namespace vkShade
