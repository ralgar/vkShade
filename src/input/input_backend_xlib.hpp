#pragma once

#include <chrono>
#include <optional>
#include <vector>

#include "input_manager.hpp"
#include "mouse_capture_controller.hpp"
#include "virtual_mouse_cursor.hpp"

typedef struct _XDisplay Display;
typedef unsigned long Window;
typedef unsigned long Cursor;

namespace vkShade
{
    class InputBackendXlib : public InputManager, public MouseCaptureBackend
    {
    public:
        InputBackendXlib(Display* display, Window window);
        ~InputBackendXlib();

        void process_events() override;
        void prepare_for_surface_replacement(Display* display, Window window);

        MouseCaptureAttempt acquire() override;
        void release() override;
        MouseCaptureStatus get_status() const override;

    private:
        Display* m_display;
        Display* m_xiObserverDisplay {nullptr};
        int      m_xiObserverOpcode {0};
        Window   m_window;
        char     m_previousKeymap[32];  // Track previous keyboard state

        // Track previous mouse state
        glm::vec2 m_prevMousePos {0.0f, 0.0f};
        bool m_prevLeftButton = false;
        bool m_prevMiddleButton = false;
        bool m_prevRightButton = false;

        struct XISelection
        {
            int deviceId;
            std::vector<unsigned char> mask;
        };

        bool m_mouseCaptured {false};
        bool m_captureRequested {false};
        bool m_restorePointerGrab {false};
        unsigned int m_applicationPointerEventMask {0};
        unsigned int m_restorePointerGrabEventMask {0};
        Display* m_captureDisplay {nullptr};
        Window m_captureWindow {0};
        Window m_topLevelWindow {0};
        Cursor m_hiddenCursor {0};
        bool m_pointerGrabbed {false};
        std::optional<bool> m_lastFocusActive;
        std::chrono::steady_clock::time_point m_nextGrabRetry {};
        VirtualMouseCursor m_virtualCursor {{0.0f, 0.0f}};
        glm::vec2 m_windowRootPosition {0.0f, 0.0f};
        glm::vec2 m_windowSize {0.0f, 0.0f};
        bool m_pointerOutsideWindow {false};
        std::optional<int> m_lastGrabFailure;
        std::vector<XISelection> m_savedXISelections;

        void handle_key_event(uint32_t keyCode, bool pressed);
        void query_mouse_state();
        void update_modifiers(unsigned int state);
        void initialize_virtual_cursor();
        bool begin_mouse_capture();
        void end_mouse_capture();
        void suspend_raw_mouse_input();
        void restore_raw_mouse_input();
        void handle_captured_event(const void* event);
        bool forward_x11_wheel_button(int button);
        int grab_pointer();
        int acquire_pointer_grab();
        void synchronize_cursor_to_pointer();
        void restore_pointer_grab();
        bool is_window_active(Display* display);
        bool is_pointer_inside_window(Display* display);
        static Window get_top_level_window(Display* display, Window window);
        void reconcile_focus();
    };
} // namespace vkShade
