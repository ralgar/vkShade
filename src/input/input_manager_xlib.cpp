#include "input_manager_xlib.hpp"

#include <spdlog/spdlog.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <xkbcommon/xkbcommon.h>

vkShade::InputManagerXlib::InputManagerXlib(Display* display)
    : m_display(display)
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
