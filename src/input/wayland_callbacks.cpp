#include "wayland_callbacks.hpp"

#include <string.h>

#include <spdlog/spdlog.h>

#include "core/service_locator.hpp"
#include "input/input_manager.hpp"
#include "input/input_manager_wayland.hpp"

// Keyboard callbacks
static void kb_keymap(void* data, wl_keyboard* kbd, uint32_t format, int32_t fd, uint32_t size)
{
    static_cast<vkShade::InputManagerWayland*>(data)->on_keyboard_keymap(format, fd, size);
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
    static_cast<vkShade::InputManagerWayland*>(data)->on_keyboard_key(key, state);
}

static void kb_modifiers(void* data, wl_keyboard* kbd, uint32_t serial,
                         uint32_t mods_depressed, uint32_t mods_latched,
                         uint32_t mods_locked, uint32_t group)
{
    static_cast<vkShade::InputManagerWayland*>(data)->on_keyboard_modifiers(mods_depressed, mods_latched, mods_locked, group);
}

// Seat callback
static void seat_capabilities(void* data, wl_seat* seat, uint32_t caps)
{
    static_cast<vkShade::InputManagerWayland*>(data)->on_seat_capabilities(seat, caps);
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
    static_cast<vkShade::InputManagerWayland*>(data)->on_registry_global(reg, name, interface, version);
}

const wl_registry_listener reg_listener = {
    registry_global, NULL
};
