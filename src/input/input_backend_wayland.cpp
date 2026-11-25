#include "input_backend_wayland.hpp"

#include <cstring>
#include <sys/mman.h>
#include <unistd.h>

#include "core/logger.hpp"
#include <xkbcommon/xkbcommon.h>

#include "mouse_button_codes.hpp"
#include "wayland_callbacks.hpp"

vkShade::InputBackendWayland::InputBackendWayland(wl_display* waylandDisplay)
{
    m_display = waylandDisplay;

    m_queue = wl_display_create_queue(m_display);

    wl_registry* reg = wl_display_get_registry(m_display);

    wl_proxy_set_queue(reinterpret_cast<wl_proxy*>(reg), m_queue);
    wl_registry_add_listener(reg, &reg_listener, this);     // Pass 'this' as void* in callbacks
    wl_display_roundtrip_queue(m_display, m_queue);

    wl_registry_destroy(reg);
}

vkShade::InputBackendWayland::~InputBackendWayland()
{
    this->release_keyboard();
    this->release_pointer();
    this->release_seat();

    if (m_queue)
        wl_event_queue_destroy(m_queue);
}

void vkShade::InputBackendWayland::bind_keyboard(wl_seat* seat)
{
    if (seat != m_seat || m_keyboard)
        return;

    m_keyboard = wl_seat_get_keyboard(m_seat);
    wl_proxy_set_queue(reinterpret_cast<wl_proxy*>(m_keyboard), m_queue);
    wl_keyboard_add_listener(m_keyboard, &kb_listener, this);

    Logger::trace("Bound to wl_keyboard");
}

void vkShade::InputBackendWayland::bind_pointer(wl_seat* seat)
{
    if (seat != m_seat || m_pointer)
        return;

    m_pointer = wl_seat_get_pointer(seat);
    wl_proxy_set_queue(reinterpret_cast<wl_proxy*>(m_pointer), m_queue);
    wl_pointer_add_listener(m_pointer, &pointer_listener, this);

    Logger::trace("Bound to wl_pointer");
}

void vkShade::InputBackendWayland::on_keyboard_key(uint32_t key, uint32_t state)
{
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
    // Single-seat only: bind the first seat and ignore any others.
    // The registry is destroyed after the initial roundtrip, so seats added
    // or removed later (global/global_remove) are not tracked.
    if (!m_seat && strcmp(interface, wl_seat_interface.name) == 0)
    {
        uint32_t seatVersion = std::min(version, 5u);
        m_seat = static_cast<wl_seat*>(wl_registry_bind(reg, name, &wl_seat_interface, seatVersion));
        wl_proxy_set_queue(reinterpret_cast<wl_proxy*>(m_seat), m_queue);
        wl_seat_add_listener(m_seat, &seat_listener, this);   // Pass 'this' as data* in callbacks
        Logger::trace("Bound to wl_seat");
    }
}

void vkShade::InputBackendWayland::on_seat_capabilities(wl_seat* seat, uint32_t caps)
{
    // Handle keyboard
    if (caps & WL_SEAT_CAPABILITY_KEYBOARD)
        this->bind_keyboard(seat);
    else
        this->release_keyboard();

    // Handle pointer
    if (caps & WL_SEAT_CAPABILITY_POINTER)
        this->bind_pointer(seat);
    else
        this->release_pointer();
}

void vkShade::InputBackendWayland::process_events()
{
    // Process Wayland events on our own queue
    if (m_display && m_queue)
        wl_display_dispatch_queue_pending(m_display, m_queue);
}

void vkShade::InputBackendWayland::release_keyboard()
{
    if (!m_keyboard)
        return;

    if (wl_proxy_get_version(reinterpret_cast<wl_proxy*>(m_keyboard)) >= WL_KEYBOARD_RELEASE_SINCE_VERSION)
        wl_keyboard_release(m_keyboard);
    else
        wl_keyboard_destroy(m_keyboard);

    m_keyboard = nullptr;
    Logger::trace("Released wl_keyboard");
}

void vkShade::InputBackendWayland::release_pointer()
{
    if (!m_pointer)
        return;

    if (wl_pointer_get_version(m_pointer) >= WL_POINTER_RELEASE_SINCE_VERSION)
        wl_pointer_release(m_pointer);
    else
        wl_pointer_destroy(m_pointer);

    m_pointer = nullptr;
    Logger::trace("Released wl_pointer");
}

void vkShade::InputBackendWayland::release_seat()
{
    if (!m_seat)
        return;

    if (wl_seat_get_version(m_seat) >= WL_SEAT_RELEASE_SINCE_VERSION)
        wl_seat_release(m_seat);
    else
        wl_seat_destroy(m_seat);

    m_seat = nullptr;
    Logger::trace("Released wl_seat");
}
