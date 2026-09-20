#include "input_backend_wayland.hpp"

#include <cstring>
#include <sys/mman.h>
#include <unistd.h>

#include "core/logger.hpp"
#include <xkbcommon/xkbcommon.h>

#include "mouse_button_codes.hpp"
#include "wayland_callbacks.hpp"

vkShade::InputBackendWayland::InputBackendWayland(wl_display* waylandDisplay)
    : m_context(waylandDisplay)
{
    if (!m_context.is_available())
        return;

    wl_registry_add_listener(m_context.get_registry(), &reg_listener, this);
    if (!m_context.synchronize())
        Logger::error("Failed to initialize Wayland input globals");
}

vkShade::InputBackendWayland::~InputBackendWayland()
{
    if (m_pointer)
    {
        if (wl_proxy_get_version(reinterpret_cast<wl_proxy*>(m_pointer)) >= WL_POINTER_RELEASE_SINCE_VERSION)
            wl_pointer_release(m_pointer);
        else
            wl_pointer_destroy(m_pointer);
    }
    if (m_keyboard)
    {
        if (wl_proxy_get_version(reinterpret_cast<wl_proxy*>(m_keyboard)) >= WL_KEYBOARD_RELEASE_SINCE_VERSION)
            wl_keyboard_release(m_keyboard);
        else
            wl_keyboard_destroy(m_keyboard);
    }
    if (m_seat)
    {
        if (wl_proxy_get_version(reinterpret_cast<wl_proxy*>(m_seat)) >= WL_SEAT_RELEASE_SINCE_VERSION)
            wl_seat_release(m_seat);
        else
            wl_seat_destroy(m_seat);
    }
}

void vkShade::InputBackendWayland::on_keyboard_key(
    uint32_t serial, uint32_t key, uint32_t state)
{
    m_context.get_state()->set_input_serial(serial);
    uint32_t keyCode = key + 8;  // Wayland uses evdev codes, XKB expects +8
    bool pressed = (state == WL_KEYBOARD_KEY_STATE_PRESSED);

    handle_keyboard_event(keyCode, pressed);
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
}

void vkShade::InputBackendWayland::on_pointer_leave(wl_surface* surface)
{
}

void vkShade::InputBackendWayland::on_pointer_motion(uint32_t time, wl_fixed_t x, wl_fixed_t y)
{
    float fx = wl_fixed_to_double(x);
    float fy = wl_fixed_to_double(y);
    handle_mouse_motion_event(fx, fy);
}

void vkShade::InputBackendWayland::on_pointer_button(uint32_t serial, uint32_t time, uint32_t button, uint32_t state)
{
    m_context.get_state()->set_input_serial(serial);
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
}

void vkShade::InputBackendWayland::on_pointer_axis_discrete(uint32_t axis, int32_t discrete)
{
    switch (axis)
    {
        case WL_POINTER_AXIS_VERTICAL_SCROLL:
            handle_mouse_wheel_event(0.0f, -static_cast<float>(discrete));
            break;

        case WL_POINTER_AXIS_HORIZONTAL_SCROLL:
            handle_mouse_wheel_event(-static_cast<float>(discrete), 0.0f);
            break;
    }
}

void vkShade::InputBackendWayland::on_registry_global(wl_registry* reg, uint32_t name, const char* interface, uint32_t version)
{
    if (strcmp(interface, wl_seat_interface.name) == 0)
    {
        uint32_t seatVersion = std::min(version, 5u);
        m_seat = static_cast<wl_seat*>(wl_registry_bind(reg, name, &wl_seat_interface, seatVersion));
        m_context.assign_queue(reinterpret_cast<wl_proxy*>(m_seat));
        wl_seat_add_listener(m_seat, &seat_listener, this);   // Pass 'this' as data* in callbacks
        Logger::trace("Bound to wl_seat");
    }
}

void vkShade::InputBackendWayland::on_seat_capabilities(wl_seat* seat, uint32_t caps)
{
    // Add keyboard handling
    if (caps & WL_SEAT_CAPABILITY_KEYBOARD)
    {
        m_keyboard = wl_seat_get_keyboard(seat);
        m_context.assign_queue(reinterpret_cast<wl_proxy*>(m_keyboard));
        wl_keyboard_add_listener(m_keyboard, &kb_listener, this);  // Pass 'this' as data* in callbacks
        Logger::trace("Bound to wl_keyboard");
    }

    // Add pointer handling
    if (caps & WL_SEAT_CAPABILITY_POINTER)
    {
        m_pointer = wl_seat_get_pointer(seat);
        m_context.assign_queue(reinterpret_cast<wl_proxy*>(m_pointer));
        wl_pointer_add_listener(m_pointer, &pointer_listener, this);
        Logger::trace("Bound to wl_pointer");
    }
}

void vkShade::InputBackendWayland::process_events()
{
    m_context.dispatch_pending();
}

std::shared_ptr<vkShade::Platform::WaylandClientState>
vkShade::InputBackendWayland::get_wayland_client_state() const
{
    return m_context.get_state();
}
