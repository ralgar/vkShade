#include "input_backend_xcb.hpp"

#include "core/logger.hpp"
#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <xkbcommon/xkbcommon.h>

vkShade::InputBackendXcb::InputBackendXcb(xcb_connection_t* connection, xcb_window_t window)
    : m_connection(connection),
      m_window(window)
{
    if (!m_connection)
    {
        Logger::error("[InputBackendXcb] Invalid connection");
        return;
    }

    // Select keyboard and mouse events from the window
    const uint32_t values[] = {
        XCB_EVENT_MASK_KEY_PRESS |
        XCB_EVENT_MASK_KEY_RELEASE |
        XCB_EVENT_MASK_BUTTON_PRESS |
        XCB_EVENT_MASK_BUTTON_RELEASE |
        XCB_EVENT_MASK_POINTER_MOTION
    };

    xcb_change_window_attributes(m_connection, m_window, XCB_CW_EVENT_MASK, values);
    xcb_flush(m_connection);
}

void vkShade::InputBackendXcb::process_events()
{
    if (!m_connection)
        return;

    // Collect all events for re-injection
    std::vector<xcb_generic_event_t*> events;

    xcb_generic_event_t* event;
    while ((event = xcb_poll_for_event(m_connection)))
    {
        uint8_t event_type = event->response_type & ~0x80;

        switch (event_type)
        {
            case XCB_KEY_PRESS:
            {
                auto* key_event = reinterpret_cast<xcb_key_press_event_t*>(event);
                update_modifiers(key_event->state);
                handle_key_event(key_event->detail, true);
                break;
            }
            case XCB_KEY_RELEASE:
            {
                auto* key_event = reinterpret_cast<xcb_key_release_event_t*>(event);
                update_modifiers(key_event->state);
                handle_key_event(key_event->detail, false);
                break;
            }
            case XCB_BUTTON_PRESS:
            {
                auto* button_event = reinterpret_cast<xcb_button_press_event_t*>(event);
                on_mouse_button(button_event->detail, true);
                break;
            }
            case XCB_BUTTON_RELEASE:
            {
                auto* button_event = reinterpret_cast<xcb_button_release_event_t*>(event);
                on_mouse_button(button_event->detail, false);
                break;
            }
            case XCB_MOTION_NOTIFY:
            {
                auto* motion_event = reinterpret_cast<xcb_motion_notify_event_t*>(event);
                on_mouse_motion(motion_event->event_x, motion_event->event_y);
                break;
            }
        }

        // Store event for re-injection
        events.push_back(event);
    }

    // Now re-inject all events back to the underlying application
    for (auto* stored_event : events)
    {
        xcb_send_event(m_connection, 0, m_window, XCB_EVENT_MASK_NO_EVENT, (const char*)stored_event);
        free(stored_event);
    }

    // Flush only if there are events in the queue
    if (!events.empty())
        xcb_flush(m_connection);
}

void vkShade::InputBackendXcb::handle_key_event(uint32_t keyCode, bool pressed)
{
    if (!m_xkbState)
        return;

    // Update XKB state
    if (pressed)
        xkb_state_update_key(m_xkbState, keyCode, XKB_KEY_DOWN);
    else
        xkb_state_update_key(m_xkbState, keyCode, XKB_KEY_UP);

    // Call base class to update key state map
    this->handle_keyboard_event(keyCode, pressed);
}

void vkShade::InputBackendXcb::on_mouse_button(uint8_t button, bool pressed)
{
    // XCB button codes: 1=left, 2=middle, 3=right, 4=scroll up, 5=scroll down
    MouseButton mouseButton;
    switch (button) {
        case 1: mouseButton = MouseButton::LEFT; break;
        case 2: mouseButton = MouseButton::MIDDLE; break;
        case 3: mouseButton = MouseButton::RIGHT; break;
        case 4:
            if (pressed)
                this->handle_mouse_wheel_event(0.0f, 1.0f);
            return;

        case 5:
            if (pressed)
                this->handle_mouse_wheel_event(0.0f, -1.0f);
        default:
            return;
    }

    handle_mouse_button_event(mouseButton, pressed);

    Logger::debug("[InputBackendXcb] Mouse button {} {}",
                  (int)mouseButton, pressed ? "pressed" : "released");
}

void vkShade::InputBackendXcb::on_mouse_motion(int16_t x, int16_t y)
{
    handle_mouse_motion_event(static_cast<float>(x), static_cast<float>(y));
}

void vkShade::InputBackendXcb::update_modifiers(uint16_t state)
{
    if (!m_xkbState || !m_xkbKeymap)
        return;

    xkb_mod_mask_t depressed = 0;
    xkb_mod_mask_t latched = 0;
    xkb_mod_mask_t locked = 0;

    // Map X11 modifier state to XKB
    xkb_mod_index_t shift_idx = xkb_keymap_mod_get_index(m_xkbKeymap, XKB_MOD_NAME_SHIFT);
    xkb_mod_index_t ctrl_idx = xkb_keymap_mod_get_index(m_xkbKeymap, XKB_MOD_NAME_CTRL);
    xkb_mod_index_t alt_idx = xkb_keymap_mod_get_index(m_xkbKeymap, XKB_MOD_NAME_ALT);
    xkb_mod_index_t caps_idx = xkb_keymap_mod_get_index(m_xkbKeymap, XKB_MOD_NAME_CAPS);

    if (state & XCB_MOD_MASK_SHIFT && shift_idx != XKB_MOD_INVALID)
        depressed |= (1 << shift_idx);
    if (state & XCB_MOD_MASK_CONTROL && ctrl_idx != XKB_MOD_INVALID)
        depressed |= (1 << ctrl_idx);
    if (state & XCB_MOD_MASK_1 && alt_idx != XKB_MOD_INVALID)
        depressed |= (1 << alt_idx);
    if (state & XCB_MOD_MASK_LOCK && caps_idx != XKB_MOD_INVALID)
        locked |= (1 << caps_idx);

    xkb_state_update_mask(m_xkbState, depressed, latched, locked, 0, 0, 0);
}
