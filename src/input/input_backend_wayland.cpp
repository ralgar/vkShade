#include "input_backend_wayland.hpp"

#include <cstring>
#include <sys/mman.h>
#include <unistd.h>

#include "core/logger.hpp"
#include <xkbcommon/xkbcommon.h>

#include "mouse_button_codes.hpp"
#include "wayland_callbacks.hpp"
#include "pointer-constraints-unstable-v1-client-protocol.h"
#include "relative-pointer-unstable-v1-client-protocol.h"

namespace
{
    const zwp_relative_pointer_v1_listener relativePointerListener = {
        .relative_motion = [](void* data, zwp_relative_pointer_v1*, uint32_t, uint32_t,
                              wl_fixed_t x, wl_fixed_t y, wl_fixed_t, wl_fixed_t)
        {
            static_cast<vkShade::InputBackendWayland*>(data)->on_relative_motion(x, y);
        },
    };

    const zwp_locked_pointer_v1_listener lockedPointerListener = {
        .locked = [](void* data, zwp_locked_pointer_v1*)
        {
            static_cast<vkShade::InputBackendWayland*>(data)->on_pointer_locked();
        },
        .unlocked = [](void* data, zwp_locked_pointer_v1*)
        {
            static_cast<vkShade::InputBackendWayland*>(data)->on_pointer_unlocked();
        },
    };
} // namespace

vkShade::InputBackendWayland::InputBackendWayland(wl_display* waylandDisplay, wl_surface* waylandSurface)
    : m_context(waylandDisplay)
    , m_display(waylandDisplay)
    , m_surface(waylandSurface)
{
    if (!m_context.is_available())
        return;

    wl_registry_add_listener(m_context.get_registry(), &reg_listener, this);
    if (!m_context.synchronize())
        Logger::error("Failed to initialize Wayland input globals");

    initialize_mouse_capture(*this);
}

vkShade::InputBackendWayland::~InputBackendWayland()
{
    shutdown_mouse_capture();
    destroy_relative_pointer();
    if (m_relativePointerManager)
        zwp_relative_pointer_manager_v1_destroy(m_relativePointerManager);
    if (m_pointerConstraints)
        zwp_pointer_constraints_v1_destroy(m_pointerConstraints);
    destroy_pointer();
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

bool vkShade::InputBackendWayland::uses_display(wl_display* display) const
{
    return m_display == display;
}

void vkShade::InputBackendWayland::set_surface(wl_surface* surface)
{
    if (m_surface == surface)
        return;

    release();
    m_surface = surface;
    m_pointerFocused = false;
    m_pointerEnterSerial = 0;
    m_hasPointerPosition = false;
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

void vkShade::InputBackendWayland::on_pointer_enter(
    uint32_t serial, wl_surface* surface, wl_fixed_t x, wl_fixed_t y)
{
    if (surface != m_surface)
        return;

    m_pointerFocused = true;
    m_pointerEnterSerial = serial;
    const float fx = wl_fixed_to_double(x);
    const float fy = wl_fixed_to_double(y);
    m_lastPointerPosition = {fx, fy};
    m_hasPointerPosition = true;
    handle_mouse_motion_event(fx, fy);
}

void vkShade::InputBackendWayland::on_pointer_leave(wl_surface* surface)
{
    if (surface == m_surface)
        m_pointerFocused = false;
}

void vkShade::InputBackendWayland::on_pointer_motion(uint32_t time, wl_fixed_t x, wl_fixed_t y)
{
    if (!m_pointerFocused)
        return;

    const float fx = wl_fixed_to_double(x);
    const float fy = wl_fixed_to_double(y);
    m_lastPointerPosition = {fx, fy};
    m_hasPointerPosition = true;
    if (m_captureStatus != MouseCaptureStatus::Active)
        handle_mouse_motion_event(fx, fy);
}

void vkShade::InputBackendWayland::on_pointer_button(uint32_t serial, uint32_t time, uint32_t button, uint32_t state)
{
    if (!m_pointerFocused)
        return;

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
    else if (strcmp(interface, zwp_relative_pointer_manager_v1_interface.name) == 0)
    {
        m_relativePointerManager = static_cast<zwp_relative_pointer_manager_v1*>(
            wl_registry_bind(reg, name, &zwp_relative_pointer_manager_v1_interface, 1));
        m_context.assign_queue(reinterpret_cast<wl_proxy*>(m_relativePointerManager));
        create_relative_pointer();
        Logger::trace("Bound to zwp_relative_pointer_manager_v1");
    }
    else if (strcmp(interface, zwp_pointer_constraints_v1_interface.name) == 0)
    {
        m_pointerConstraints = static_cast<zwp_pointer_constraints_v1*>(
            wl_registry_bind(reg, name, &zwp_pointer_constraints_v1_interface, 1));
        m_context.assign_queue(reinterpret_cast<wl_proxy*>(m_pointerConstraints));
        Logger::trace("Bound to zwp_pointer_constraints_v1");
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
    if ((caps & WL_SEAT_CAPABILITY_POINTER) && !m_pointer)
    {
        m_pointer = wl_seat_get_pointer(seat);
        m_context.assign_queue(reinterpret_cast<wl_proxy*>(m_pointer));
        wl_pointer_add_listener(m_pointer, &pointer_listener, this);
        create_relative_pointer();
        Logger::trace("Bound to wl_pointer");
    }
    else if (!(caps & WL_SEAT_CAPABILITY_POINTER) && m_pointer)
    {
        release();
        destroy_relative_pointer();
        destroy_pointer();
    }
}

void vkShade::InputBackendWayland::on_relative_motion(wl_fixed_t x, wl_fixed_t y)
{
    if (m_captureStatus != MouseCaptureStatus::Active)
        return;

    const glm::vec2 position = m_virtualCursor.observe_relative_motion({
        static_cast<float>(wl_fixed_to_double(x)),
        static_cast<float>(wl_fixed_to_double(y)),
    });
    handle_mouse_motion_event(position.x, position.y);
}

void vkShade::InputBackendWayland::on_pointer_locked()
{
    m_captureStatus = MouseCaptureStatus::Active;
    hide_native_cursor();
    Logger::debug("[Wayland] Mouse input captured for overlay");
}

void vkShade::InputBackendWayland::on_pointer_unlocked()
{
    if (m_lockedPointer)
        m_captureStatus = MouseCaptureStatus::Suspended;
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

void vkShade::InputBackendWayland::set_pointer_bounds(glm::vec2 bounds)
{
    m_pointerBounds = bounds;
    m_virtualCursor.set_bounds(bounds);
}

vkShade::MouseCaptureAttempt vkShade::InputBackendWayland::acquire()
{
    if (!m_surface || !m_relativePointerManager || !m_pointerConstraints)
    {
        m_captureStatus = MouseCaptureStatus::Unavailable;
        return {m_captureStatus, false};
    }
    if (!m_pointer)
        return {MouseCaptureStatus::Inactive, true};
    if (m_lockedPointer)
        return {m_captureStatus, false};

    create_relative_pointer();
    if (!m_relativePointer)
        return {MouseCaptureStatus::Inactive, true};

    const glm::vec2 initialPosition = m_hasPointerPosition
        ? m_lastPointerPosition
        : m_pointerBounds * 0.5f;
    m_virtualCursor.reset(initialPosition);
    handle_mouse_motion_event(initialPosition.x, initialPosition.y);

    m_lockedPointer = zwp_pointer_constraints_v1_lock_pointer(
        m_pointerConstraints, m_surface, m_pointer, nullptr,
        ZWP_POINTER_CONSTRAINTS_V1_LIFETIME_PERSISTENT);
    if (!m_lockedPointer)
        return {MouseCaptureStatus::Inactive, true};

    m_context.assign_queue(reinterpret_cast<wl_proxy*>(m_lockedPointer));
    zwp_locked_pointer_v1_add_listener(m_lockedPointer, &lockedPointerListener, this);
    m_captureStatus = MouseCaptureStatus::Pending;
    m_context.flush();
    return {m_captureStatus, false};
}

void vkShade::InputBackendWayland::release()
{
    if (!m_lockedPointer)
    {
        m_captureStatus = MouseCaptureStatus::Inactive;
        return;
    }

    zwp_locked_pointer_v1_destroy(m_lockedPointer);
    m_lockedPointer = nullptr;
    m_captureStatus = MouseCaptureStatus::Inactive;
    m_context.flush();
    Logger::debug("[Wayland] Mouse input returned to application");
}

vkShade::MouseCaptureStatus vkShade::InputBackendWayland::get_status() const
{
    return m_captureStatus;
}

void vkShade::InputBackendWayland::create_relative_pointer()
{
    if (m_relativePointer || !m_relativePointerManager || !m_pointer)
        return;

    m_relativePointer = zwp_relative_pointer_manager_v1_get_relative_pointer(
        m_relativePointerManager, m_pointer);
    m_context.assign_queue(reinterpret_cast<wl_proxy*>(m_relativePointer));
    zwp_relative_pointer_v1_add_listener(m_relativePointer, &relativePointerListener, this);
}

void vkShade::InputBackendWayland::destroy_relative_pointer()
{
    if (!m_relativePointer)
        return;

    zwp_relative_pointer_v1_destroy(m_relativePointer);
    m_relativePointer = nullptr;
}

void vkShade::InputBackendWayland::destroy_pointer()
{
    if (!m_pointer)
        return;

    if (wl_proxy_get_version(reinterpret_cast<wl_proxy*>(m_pointer)) >= WL_POINTER_RELEASE_SINCE_VERSION)
        wl_pointer_release(m_pointer);
    else
        wl_pointer_destroy(m_pointer);
    m_pointer = nullptr;
}

void vkShade::InputBackendWayland::hide_native_cursor()
{
    if (m_pointer && m_pointerFocused && m_pointerEnterSerial != 0)
    {
        wl_pointer_set_cursor(m_pointer, m_pointerEnterSerial, nullptr, 0, 0);
        m_context.flush();
    }
}
