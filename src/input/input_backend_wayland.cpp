#include "input_backend_wayland.hpp"

#include <cstring>
#include <sys/mman.h>
#include <unistd.h>

#include <spdlog/spdlog.h>
#include <xkbcommon/xkbcommon.h>

#include "mouse_button_codes.hpp"
#include "wayland_callbacks.hpp"

vkShade::InputBackendWayland::InputBackendWayland(wl_display* waylandDisplay)
{
    // Set up input
    m_display = waylandDisplay;
    wl_registry* reg = wl_display_get_registry(m_display);
    wl_registry_add_listener(reg, &reg_listener, this);     // Pass 'this' as void* in callbacks
    wl_display_roundtrip(m_display);  // Get globals
}

void vkShade::InputBackendWayland::on_keyboard_key(uint32_t key, uint32_t state)
{
    uint32_t keycode = key + 8;  // Wayland uses evdev codes, XKB expects +8
    xkb_keysym_t sym = xkb_state_key_get_one_sym(m_xkbState, keycode);
    bool pressed = (state == WL_KEYBOARD_KEY_STATE_PRESSED);

    handle_keyboard_event(sym, pressed);
}

void vkShade::InputBackendWayland::on_keyboard_keymap(uint32_t format, int32_t fd, uint32_t size)
{
    if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1)
    {
        close(fd);
        return;
    }

    char* map = (char*)mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    xkb_keymap* keymap = xkb_keymap_new_from_string(m_xkbContext, map,
        XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
    munmap(map, size);
    close(fd);

    if (m_xkbState) xkb_state_unref(m_xkbState);
    m_xkbState = xkb_state_new(keymap);
    xkb_keymap_unref(keymap);
}

void vkShade::InputBackendWayland::on_keyboard_modifiers(uint32_t modsDepressed,
                                                         uint32_t modsLatched,
                                                         uint32_t modsLocked,
                                                         uint32_t group)
{
    if (m_xkbState)
    {
        xkb_state_update_mask(m_xkbState, modsDepressed, modsLatched, modsLocked, 0, 0, group);
    }
}

void vkShade::InputBackendWayland::on_pointer_enter(wl_surface* surface, wl_fixed_t x, wl_fixed_t y)
{
    float fx = wl_fixed_to_double(x);
    float fy = wl_fixed_to_double(y);
    handle_mouse_motion_event(fx, fy);
    spdlog::trace("Pointer entered at ({}, {})", fx, fy);
}

void vkShade::InputBackendWayland::on_pointer_leave(wl_surface* surface)
{
    spdlog::trace("Pointer left surface");
}

void vkShade::InputBackendWayland::on_pointer_motion(uint32_t time, wl_fixed_t x, wl_fixed_t y)
{
    float fx = wl_fixed_to_double(x);
    float fy = wl_fixed_to_double(y);
    handle_mouse_motion_event(fx, fy);
}

void vkShade::InputBackendWayland::on_pointer_button(uint32_t serial, uint32_t time, uint32_t button, uint32_t state)
{
    // Wayland button codes: BTN_LEFT=0x110, BTN_RIGHT=0x111, BTN_MIDDLE=0x112
    MouseButton mouseButton;
    switch (button)
    {
        case 0x110: mouseButton = MouseButton::LEFT; break;
        case 0x111: mouseButton = MouseButton::RIGHT; break;
        case 0x112: mouseButton = MouseButton::MIDDLE; break;
        default: return;  // Unknown button
    }

    bool pressed = (state == WL_POINTER_BUTTON_STATE_PRESSED);
    handle_mouse_button_event(mouseButton, pressed);
}

void vkShade::InputBackendWayland::on_pointer_axis(uint32_t time, uint32_t axis, wl_fixed_t value)
{
    // Handle scroll wheel
    float scroll = wl_fixed_to_double(value);
    spdlog::trace("Scroll axis {} value {}", axis, scroll);
}

void vkShade::InputBackendWayland::on_registry_global(wl_registry* reg, uint32_t name, const char* interface, uint32_t version)
{
    if (strcmp(interface, wl_seat_interface.name) == 0)
    {
        wl_seat* seat = static_cast<wl_seat*>(wl_registry_bind(reg, name, &wl_seat_interface, 1));
        wl_seat_add_listener(seat, &seat_listener, this);   // Pass 'this' as data* in callbacks
        spdlog::trace("Bound to wl_seat");
    }
}

void vkShade::InputBackendWayland::on_seat_capabilities(wl_seat* seat, uint32_t caps)
{
    // Add keyboard handling
    if (caps & WL_SEAT_CAPABILITY_KEYBOARD)
    {
        m_keyboard = wl_seat_get_keyboard(seat);
        wl_keyboard_add_listener(m_keyboard, &kb_listener, this);  // Pass 'this' as data* in callbacks
        spdlog::trace("Bound to wl_keyboard");
    }

    // Add pointer handling
    if (caps & WL_SEAT_CAPABILITY_POINTER)
    {
        m_pointer = wl_seat_get_pointer(seat);
        wl_pointer_add_listener(m_pointer, &pointer_listener, this);
        spdlog::trace("Bound to wl_pointer");
    }
}

void vkShade::InputBackendWayland::process_events()
{
    // Process Wayland events on default queue (non-blocking)
    if (m_display)
    {
        wl_display_dispatch_pending(m_display);  // No queue parameter
    }
}
