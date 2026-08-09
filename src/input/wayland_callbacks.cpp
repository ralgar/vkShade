#include "wayland_callbacks.hpp"

#include <string.h>

#include "input/input_backend_wayland.hpp"

// Keyboard callbacks
static void kb_keymap(void* data, wl_keyboard* kbd, uint32_t format, int32_t fd, uint32_t size)
{
    static_cast<vkShade::InputBackendWayland*>(data)->on_keyboard_keymap(format, fd, size);
}

static void kb_enter(void* data, wl_keyboard* kbd, uint32_t serial, wl_surface* surf, wl_array* keys)
{
}

static void kb_leave(void* data, wl_keyboard* kbd, uint32_t serial, wl_surface* surf)
{
    // TODO: "Unpress" all keys here?
}

static void kb_key(void* data, wl_keyboard* kbd, uint32_t serial, uint32_t time, uint32_t key, uint32_t state)
{
    static_cast<vkShade::InputBackendWayland*>(data)->on_keyboard_key(key, state);
}

static void kb_modifiers(void* data, wl_keyboard* kbd, uint32_t serial,
                         uint32_t mods_depressed, uint32_t mods_latched,
                         uint32_t mods_locked, uint32_t group)
{
    static_cast<vkShade::InputBackendWayland*>(data)->on_keyboard_modifiers(mods_depressed, mods_latched, mods_locked, group);
}

// Seat callback
static void seat_capabilities(void* data, wl_seat* seat, uint32_t caps)
{
    static_cast<vkShade::InputBackendWayland*>(data)->on_seat_capabilities(seat, caps);
}

// Listener structs
const wl_keyboard_listener kb_listener = {
    kb_keymap, kb_enter, kb_leave, kb_key, kb_modifiers
};

const wl_seat_listener seat_listener = {
    seat_capabilities, NULL
};

// Registry callback
static void registry_global(void* data, wl_registry* reg, uint32_t name, const char* interface, uint32_t version)
{
    static_cast<vkShade::InputBackendWayland*>(data)->on_registry_global(reg, name, interface, version);
}

const wl_registry_listener reg_listener = {
    registry_global, NULL
};

// Pointer callbacks
void pointer_enter_handler(void* data, wl_pointer* pointer, uint32_t serial,
                          wl_surface* surface, wl_fixed_t x, wl_fixed_t y) {
    auto* manager = static_cast<vkShade::InputBackendWayland*>(data);
    manager->on_pointer_enter(surface, x, y);
}

void pointer_leave_handler(void* data, wl_pointer* pointer, uint32_t serial,
                          wl_surface* surface) {
    auto* manager = static_cast<vkShade::InputBackendWayland*>(data);
    manager->on_pointer_leave(surface);
}

void pointer_motion_handler(void* data, wl_pointer* pointer, uint32_t time,
                           wl_fixed_t x, wl_fixed_t y) {
    auto* manager = static_cast<vkShade::InputBackendWayland*>(data);
    manager->on_pointer_motion(time, x, y);
}

void pointer_button_handler(void* data, wl_pointer* pointer, uint32_t serial,
                           uint32_t time, uint32_t button, uint32_t state) {
    auto* manager = static_cast<vkShade::InputBackendWayland*>(data);
    manager->on_pointer_button(serial, time, button, state);
}

void pointer_axis_handler(void* data, wl_pointer* pointer, uint32_t time,
                         uint32_t axis, wl_fixed_t value) {
    auto* manager = static_cast<vkShade::InputBackendWayland*>(data);
    manager->on_pointer_axis(time, axis, value);
}

void pointer_frame_handler(void* data, wl_pointer* pointer) {
    // Frame event - commits pointer events
}

const wl_pointer_listener pointer_listener = {
    .enter = pointer_enter_handler,
    .leave = pointer_leave_handler,
    .motion = pointer_motion_handler,
    .button = pointer_button_handler,
    .axis = pointer_axis_handler,
    .frame = pointer_frame_handler,
    .axis_source = [](void*, wl_pointer*, uint32_t){},
    .axis_stop = [](void*, wl_pointer*, uint32_t, uint32_t){},
    .axis_discrete = [](void*, wl_pointer*, uint32_t, int32_t){},
};
