#pragma once

#include <wayland-client.h>

extern const wl_keyboard_listener kb_listener;
extern const wl_pointer_listener pointer_listener;
extern const wl_registry_listener reg_listener;
extern const wl_seat_listener seat_listener;
