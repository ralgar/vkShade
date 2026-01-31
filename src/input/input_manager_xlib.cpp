#include "input_manager_xlib.hpp"

#include <spdlog/spdlog.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <xkbcommon/xkbcommon.h>

vkShade::InputManagerXlib::InputManagerXlib(Display* display, Window window)
    : m_display(display),
      m_window(window)
{
    if (!m_display)
    {
        spdlog::error("[Xlib] Invalid display");
        return;
    }

    // Initialize previous key states
    std::memset(m_previousKeymap, 0, sizeof(m_previousKeymap));
}

vkShade::InputManagerXlib::~InputManagerXlib()
{
    if (m_xkbState)
        xkb_state_unref(m_xkbState);
    if (m_xkbKeymap)
        xkb_keymap_unref(m_xkbKeymap);
    if (m_xkbContext)
        xkb_context_unref(m_xkbContext);
}

void vkShade::InputManagerXlib::process_events()
{
    if (!m_display || !m_xkbState)
        return;

    // Query current keyboard state without consuming events
    char keymap[32];
    XQueryKeymap(m_display, keymap);

    // Check each key for state changes
    for (int keycode = 8; keycode < 256; keycode++)  // X11 keycodes start at 8
    {
        int byte = keycode / 8;
        int bit = keycode % 8;

        bool currently_pressed = (keymap[byte] & (1 << bit)) != 0;
        bool previously_pressed = (m_previousKeymap[byte] & (1 << bit)) != 0;

        if (currently_pressed != previously_pressed)
        {
            handle_key_event(keycode, currently_pressed);
        }
    }

    // Update previous state
    std::memcpy(m_previousKeymap, keymap, sizeof(keymap));

    // Query mouse state
    query_mouse_state();
}

void vkShade::InputManagerXlib::handle_key_event(uint32_t keyCode, bool pressed)
{
    if (!m_xkbState)
        return;

    // Get keysym from XKB
    xkb_keysym_t keysym = xkb_state_key_get_one_sym(m_xkbState, keyCode);

    // Update XKB state with this key event
    if (pressed)
    {
        xkb_state_update_key(m_xkbState, keyCode, XKB_KEY_DOWN);
    }
    else
    {
        xkb_state_update_key(m_xkbState, keyCode, XKB_KEY_UP);
    }

    // Call base class to update key state map
    this->handle_keyboard_event(keysym, pressed);
}


void vkShade::InputManagerXlib::query_mouse_state()
{
    Window root_return, child_return;
    int root_x, root_y, win_x, win_y;
    unsigned int mask_return;

    // Query pointer position and button state
    Bool result = XQueryPointer(m_display, m_window,
                                &root_return, &child_return,
                                &root_x, &root_y,
                                &win_x, &win_y,
                                &mask_return);

    if (!result)
        return;  // Pointer not in our window

    // Current state
    glm::vec2 currentPos(static_cast<float>(win_x), static_cast<float>(win_y));
    bool left_pressed = (mask_return & Button1Mask) != 0;
    bool middle_pressed = (mask_return & Button2Mask) != 0;
    bool right_pressed = (mask_return & Button3Mask) != 0;

    // Only update if position changed
    if (currentPos != m_prevMousePos)
    {
        handle_mouse_motion_event(currentPos.x, currentPos.y);
        m_prevMousePos = currentPos;
    }

    // Only update if button state changed
    if (left_pressed != m_prevLeftButton)
    {
        handle_mouse_button_event(MouseButton::LEFT, left_pressed);
        m_prevLeftButton = left_pressed;
    }

    if (middle_pressed != m_prevMiddleButton)
    {
        handle_mouse_button_event(MouseButton::MIDDLE, middle_pressed);
        m_prevMiddleButton = middle_pressed;
    }

    if (right_pressed != m_prevRightButton)
    {
        handle_mouse_button_event(MouseButton::RIGHT, right_pressed);
        m_prevRightButton = right_pressed;
    }
}
