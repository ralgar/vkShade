#pragma once

#include "input_manager.hpp"
#include "mouse_capture_controller.hpp"
#include "platform/linux/wayland_client_context.hpp"
#include "virtual_mouse_cursor.hpp"

#include <wayland-client.h>

struct zwp_locked_pointer_v1;
struct zwp_pointer_constraints_v1;
struct zwp_relative_pointer_manager_v1;
struct zwp_relative_pointer_v1;

namespace vkShade
{
    class InputBackendWayland : public InputManager, public MouseCaptureBackend
    {
    public:
        InputBackendWayland(wl_display* waylandDisplay, wl_surface* waylandSurface);
        ~InputBackendWayland() override;

        bool uses_display(wl_display* display) const;
        void set_surface(wl_surface* surface);

        // These get called by the static C callbacks
        void on_registry_global(wl_registry* reg, uint32_t name,
                                const char* interface, uint32_t version);
        void on_seat_capabilities(wl_seat* seat, uint32_t caps);
        void on_keyboard_keymap(uint32_t format, int32_t fd, uint32_t size);
        void on_keyboard_key(uint32_t serial, uint32_t key, uint32_t state);
        void on_keyboard_modifiers(uint32_t modsDepressed, uint32_t modsLatched,
                                   uint32_t modsLocked, uint32_t group);

        // Pointer callbacks
        void on_pointer_enter(uint32_t serial, wl_surface* surface, wl_fixed_t x, wl_fixed_t y);
        void on_pointer_leave(wl_surface* surface);
        void on_pointer_motion(uint32_t time, wl_fixed_t x, wl_fixed_t y);
        void on_pointer_button(uint32_t serial, uint32_t time, uint32_t button, uint32_t state);
        void on_pointer_axis(uint32_t time, uint32_t axis, wl_fixed_t value);
        void on_pointer_axis_discrete(uint32_t axis, int32_t discrete);
        void on_relative_motion(wl_fixed_t x, wl_fixed_t y);
        void on_pointer_locked();
        void on_pointer_unlocked();

        void process_events() override;
        std::shared_ptr<Platform::WaylandClientState> get_wayland_client_state() const override;
        void set_pointer_bounds(glm::vec2 bounds) override;

        MouseCaptureAttempt acquire() override;
        void release() override;
        MouseCaptureStatus get_status() const override;
        bool requires_pointer_constraint_permission() const override { return true; }

    private:
        Platform::WaylandClientContext m_context;
        wl_display*                    m_display = nullptr;
        wl_surface*                    m_surface = nullptr;
        wl_seat*                       m_seat = nullptr;
        wl_keyboard*                   m_keyboard = nullptr;
        wl_pointer*                    m_pointer = nullptr;
        zwp_relative_pointer_manager_v1* m_relativePointerManager = nullptr;
        zwp_pointer_constraints_v1*      m_pointerConstraints = nullptr;
        zwp_relative_pointer_v1*         m_relativePointer = nullptr;
        zwp_locked_pointer_v1*           m_lockedPointer = nullptr;
        VirtualMouseCursor               m_virtualCursor {{0.0f, 0.0f}};
        glm::vec2                        m_pointerBounds {0.0f, 0.0f};
        glm::vec2                        m_lastPointerPosition {0.0f, 0.0f};
        MouseCaptureStatus               m_captureStatus {MouseCaptureStatus::Inactive};
        uint32_t                         m_pointerEnterSerial = 0;
        bool                             m_hasPointerPosition = false;
        bool                             m_pointerFocused = false;

        void create_relative_pointer();
        void destroy_relative_pointer();
        void destroy_pointer();
        void hide_native_cursor();
    };
}// namespace vkShade
