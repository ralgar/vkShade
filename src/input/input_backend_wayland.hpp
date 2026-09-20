#pragma once

#include "input_manager.hpp"
#include "platform/linux/wayland_client_context.hpp"

#include <wayland-client.h>

namespace vkShade
{
    class InputBackendWayland : public InputManager
    {
    public:
        InputBackendWayland(wl_display* waylandDisplay);
        ~InputBackendWayland() override;

        // These get called by the static C callbacks
        void on_registry_global(wl_registry* reg, uint32_t name,
                                const char* interface, uint32_t version);
        void on_seat_capabilities(wl_seat* seat, uint32_t caps);
        void on_keyboard_keymap(uint32_t format, int32_t fd, uint32_t size);
        void on_keyboard_key(uint32_t serial, uint32_t key, uint32_t state);
        void on_keyboard_modifiers(uint32_t modsDepressed, uint32_t modsLatched,
                                   uint32_t modsLocked, uint32_t group);

        // Pointer callbacks
        void on_pointer_enter(wl_surface* surface, wl_fixed_t x, wl_fixed_t y);
        void on_pointer_leave(wl_surface* surface);
        void on_pointer_motion(uint32_t time, wl_fixed_t x, wl_fixed_t y);
        void on_pointer_button(uint32_t serial, uint32_t time, uint32_t button, uint32_t state);
        void on_pointer_axis(uint32_t time, uint32_t axis, wl_fixed_t value);
        void on_pointer_axis_discrete(uint32_t axis, int32_t discrete);

        void process_events() override;
        std::shared_ptr<Platform::WaylandClientState> get_wayland_client_state() const override;

    private:
        Platform::WaylandClientContext m_context;
        wl_seat*                       m_seat = nullptr;
        wl_keyboard*                   m_keyboard = nullptr;
        wl_pointer*                    m_pointer = nullptr;
    };
}// namespace vkShade
