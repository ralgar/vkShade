#include "input_backend_xlib.hpp"

#include <algorithm>
#include <cstring>

#include "core/logger.hpp"
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XInput2.h>
#include <X11/keysym.h>
#include <xkbcommon/xkbcommon.h>

namespace
{
    constexpr unsigned int OVERLAY_POINTER_EVENT_MASK = PointerMotionMask
                                                      | ButtonPressMask
                                                      | ButtonReleaseMask;
    constexpr unsigned int ALL_POINTER_EVENT_MASKS = OVERLAY_POINTER_EVENT_MASK
                                                   | EnterWindowMask
                                                   | LeaveWindowMask
                                                   | PointerMotionHintMask
                                                   | Button1MotionMask
                                                   | Button2MotionMask
                                                   | Button3MotionMask
                                                   | Button4MotionMask
                                                   | Button5MotionMask
                                                   | ButtonMotionMask;
}

vkShade::InputBackendXlib::InputBackendXlib(Display* display, Window window)
    : m_display(display),
      m_window(window)
{
    if (!m_display)
    {
        Logger::error("[Xlib] Invalid display");
        return;
    }

    // Initialize previous key states
    std::memset(m_previousKeymap, 0, sizeof(m_previousKeymap));

    // Observe raw wheel button events without selecting exclusive core button
    // events from the application's window.
    m_wheelDisplay = XOpenDisplay(DisplayString(m_display));
    if (m_wheelDisplay)
    {
        m_topLevelWindow = get_top_level_window(m_wheelDisplay, m_window);
        int event = 0;
        int error = 0;
        int major = 2;
        int minor = 0;
        const bool xi2Available = XQueryExtension(m_wheelDisplay, "XInputExtension",
            &m_wheelXiOpcode, &event, &error) && XIQueryVersion(m_wheelDisplay, &major, &minor) == Success;

        if (xi2Available)
        {
            unsigned char mask[XIMaskLen(XI_RawButtonPress)] {};
            XISetMask(mask, XI_RawButtonPress);
            XIEventMask eventMask {
                .deviceid = XIAllMasterDevices,
                .mask_len = static_cast<int>(sizeof(mask)),
                .mask = mask,
            };
            XISelectEvents(m_wheelDisplay, DefaultRootWindow(m_wheelDisplay), &eventMask, 1);
            XFlush(m_wheelDisplay);
        }
        else
        {
            Logger::warn("[Xlib] XInput2 is unavailable for mouse wheel input");
            XCloseDisplay(m_wheelDisplay);
            m_wheelDisplay = nullptr;
        }
    }
    else
    {
        Logger::warn("[Xlib] Could not open a display connection for mouse wheel input");
    }

    initialize_mouse_capture(*this);
}

vkShade::InputBackendXlib::~InputBackendXlib()
{
    shutdown_mouse_capture();
    if (m_wheelDisplay)
        XCloseDisplay(m_wheelDisplay);
}

void vkShade::InputBackendXlib::prepare_for_surface_replacement(
    Display* display, Window window)
{
    if (display != m_display)
    {
        // XI2 selections and pointer grabs belong to an X client connection.
        // Do not replay state captured from a connection that the application
        // replaced or may already have closed.
        m_savedXISelections.clear();
        m_restorePointerGrab = false;
        m_restorePointerGrabEventMask = 0;
    }

    m_display = display;
    m_window = window;

    if (!m_restorePointerGrab)
        return;

    XWindowAttributes attributes {};
    if (!m_display || !XGetWindowAttributes(m_display, m_window, &attributes))
    {
        m_restorePointerGrab = false;
        m_restorePointerGrabEventMask = 0;
        return;
    }

    m_restorePointerGrabEventMask =
        static_cast<unsigned int>(attributes.your_event_mask)
        & ALL_POINTER_EVENT_MASKS;
    if (!m_restorePointerGrabEventMask)
        m_restorePointerGrabEventMask = OVERLAY_POINTER_EVENT_MASK;
}

vkShade::MouseCaptureAttempt vkShade::InputBackendXlib::acquire()
{
    if (!m_display)
        return {MouseCaptureStatus::Unavailable, false};

    if (m_mouseCaptured)
        return {get_status(), false};

    m_mouseCaptured = begin_mouse_capture();
    if (!m_mouseCaptured)
        return {MouseCaptureStatus::Inactive, true};

    Logger::debug("[Xlib] Mouse input captured for overlay");
    return {get_status(), false};
}

void vkShade::InputBackendXlib::release()
{
    if (!m_mouseCaptured && !m_captureDisplay)
        return;

    end_mouse_capture();
    m_mouseCaptured = false;
    Logger::debug("[Xlib] Mouse input returned to application");
}

vkShade::MouseCaptureStatus vkShade::InputBackendXlib::get_status() const
{
    if (!m_mouseCaptured)
        return MouseCaptureStatus::Inactive;
    return m_pointerGrabbed ? MouseCaptureStatus::Active : MouseCaptureStatus::Suspended;
}

bool vkShade::InputBackendXlib::begin_mouse_capture()
{
    m_captureDisplay = XOpenDisplay(DisplayString(m_display));
    if (!m_captureDisplay)
    {
        Logger::warn("[Xlib] Could not open a separate display connection for overlay input");
        return false;
    }

    XSetWindowAttributes attributes {};
    attributes.override_redirect = True;
    attributes.event_mask = OVERLAY_POINTER_EVENT_MASK;

    const Window root = DefaultRootWindow(m_captureDisplay);
    m_topLevelWindow = get_top_level_window(m_captureDisplay, m_window);

    XWindowAttributes windowAttributes {};
    m_applicationPointerEventMask = 0;
    if (XGetWindowAttributes(m_display, m_window, &windowAttributes))
    {
        m_applicationPointerEventMask =
            static_cast<unsigned int>(windowAttributes.your_event_mask) & ALL_POINTER_EVENT_MASKS;
        Window child = None;
        int rootX = 0;
        int rootY = 0;
        XTranslateCoordinates(m_display, m_window, DefaultRootWindow(m_display),
                              0, 0, &rootX, &rootY, &child);

        m_windowSize = {
            static_cast<float>(windowAttributes.width),
            static_cast<float>(windowAttributes.height),
        };
        const glm::vec2 bounds {
            std::max(0.0f, m_windowSize.x - 1.0f),
            std::max(0.0f, m_windowSize.y - 1.0f),
        };
        const glm::vec2 center = m_windowSize * 0.5f;
        m_windowRootPosition = {
            static_cast<float>(rootX),
            static_cast<float>(rootY),
        };
        const glm::vec2 warpPosition {
            m_windowRootPosition.x + center.x,
            m_windowRootPosition.y + center.y,
        };
        m_virtualCursor.set_bounds(bounds);
        m_virtualCursor.reset(center, warpPosition);
        m_virtualCursor.set_warp_position(warpPosition);
        handle_mouse_motion_event(center.x, center.y);
    }

    m_captureWindow = XCreateWindow(
        m_captureDisplay, root, -1, -1, 1, 1, 0, CopyFromParent, InputOutput,
        CopyFromParent, CWOverrideRedirect | CWEventMask, &attributes);
    XMapWindow(m_captureDisplay, m_captureWindow);

    const char emptyData[] = {0};
    const Pixmap emptyPixmap =
        XCreateBitmapFromData(m_captureDisplay, m_captureWindow, emptyData, 1, 1);
    XColor emptyColor {};
    m_hiddenCursor = XCreatePixmapCursor(
        m_captureDisplay, emptyPixmap, emptyPixmap, &emptyColor, &emptyColor, 0, 0);
    XFreePixmap(m_captureDisplay, emptyPixmap);
    XSync(m_captureDisplay, False);

    const int grabStatus = acquire_pointer_grab();

    if (grabStatus != GrabSuccess)
    {
        Logger::warn("[Xlib] Could not capture pointer for overlay (grab status {})", grabStatus);
        XFreeCursor(m_captureDisplay, m_hiddenCursor);
        XDestroyWindow(m_captureDisplay, m_captureWindow);
        XCloseDisplay(m_captureDisplay);
        m_hiddenCursor = 0;
        m_captureWindow = 0;
        m_captureDisplay = nullptr;
        restore_pointer_grab();
        return false;
    }

    suspend_raw_mouse_input();
    synchronize_cursor_to_pointer();
    m_lastFocusActive.reset();
    return true;
}

void vkShade::InputBackendXlib::end_mouse_capture()
{
    if (m_captureDisplay)
    {
        XUngrabPointer(m_captureDisplay, CurrentTime);
        m_pointerGrabbed = false;

        if (m_hiddenCursor)
            XFreeCursor(m_captureDisplay, m_hiddenCursor);
        if (m_captureWindow)
            XDestroyWindow(m_captureDisplay, m_captureWindow);
        XCloseDisplay(m_captureDisplay);
        m_hiddenCursor = 0;
        m_captureWindow = 0;
        m_captureDisplay = nullptr;
    }

    restore_raw_mouse_input();
    restore_pointer_grab();
}

int vkShade::InputBackendXlib::grab_pointer()
{
    const int result = XGrabPointer(
        m_captureDisplay, m_captureWindow, False, OVERLAY_POINTER_EVENT_MASK,
        GrabModeAsync, GrabModeAsync, None, m_hiddenCursor, CurrentTime);
    XFlush(m_captureDisplay);
    m_pointerGrabbed = result == GrabSuccess;
    return result;
}

int vkShade::InputBackendXlib::acquire_pointer_grab()
{
    int result = grab_pointer();
    if (result != AlreadyGrabbed)
        return result;

    Logger::debug("[Xlib] Releasing application pointer grab for overlay");
    if (!m_restorePointerGrab)
    {
        m_restorePointerGrabEventMask = m_applicationPointerEventMask
            ? m_applicationPointerEventMask
            : OVERLAY_POINTER_EVENT_MASK;
        m_restorePointerGrab = true;
    }

    XUngrabPointer(m_display, CurrentTime);
    XSync(m_display, False);
    return grab_pointer();
}

void vkShade::InputBackendXlib::restore_pointer_grab()
{
    if (!m_restorePointerGrab)
        return;

    const int result = XGrabPointer(
        m_display, m_window, True, m_restorePointerGrabEventMask,
        GrabModeAsync, GrabModeAsync, None, None, CurrentTime);
    if (result == GrabSuccess)
        Logger::debug("[Xlib] Restored application pointer grab");
    else
        Logger::warn("[Xlib] Could not restore application pointer grab (grab status {})", result);

    m_restorePointerGrab = false;
    m_restorePointerGrabEventMask = 0;
    XFlush(m_display);
}

void vkShade::InputBackendXlib::reconcile_focus()
{
    if (!m_captureDisplay)
        return;

    const bool active = is_window_active(m_captureDisplay);
    const bool pointerWasGrabbed = m_pointerGrabbed;
    if (!active && m_pointerGrabbed)
    {
        XUngrabPointer(m_captureDisplay, CurrentTime);
        XFlush(m_captureDisplay);
        m_pointerGrabbed = false;
    }
    else if (active && !m_pointerGrabbed)
    {
        const auto now = std::chrono::steady_clock::now();
        if (now >= m_nextGrabRetry)
        {
            const int result = acquire_pointer_grab();
            m_nextGrabRetry = now + std::chrono::milliseconds(100);
            if (result != GrabSuccess)
            {
                Logger::debug(
                    "[Xlib] Could not reacquire overlay pointer grab (grab status {})", result);
            }
        }
    }

    if (!pointerWasGrabbed && m_pointerGrabbed)
        synchronize_cursor_to_pointer();

    if (m_lastFocusActive && active != *m_lastFocusActive)
    {
        Logger::debug("[Xlib] Overlay pointer grab {} after focus change",
                      active ? (m_pointerGrabbed ? "restored" : "pending") : "released");
    }
    m_lastFocusActive = active;
}

void vkShade::InputBackendXlib::synchronize_cursor_to_pointer()
{
    XWindowAttributes attributes {};
    if (!XGetWindowAttributes(m_captureDisplay, m_window, &attributes))
        return;

    Window translatedChild = None;
    int windowRootX = 0;
    int windowRootY = 0;
    XTranslateCoordinates(
        m_captureDisplay, m_window, DefaultRootWindow(m_captureDisplay),
        0, 0, &windowRootX, &windowRootY, &translatedChild);

    m_windowRootPosition = {
        static_cast<float>(windowRootX),
        static_cast<float>(windowRootY),
    };
    m_windowSize = {
        static_cast<float>(attributes.width),
        static_cast<float>(attributes.height),
    };
    const glm::vec2 bounds {
        std::max(0.0f, m_windowSize.x - 1.0f),
        std::max(0.0f, m_windowSize.y - 1.0f),
    };
    const glm::vec2 center = m_windowSize * 0.5f;
    m_virtualCursor.set_bounds(bounds);
    m_virtualCursor.set_warp_position(m_windowRootPosition + center);

    Window root = None;
    Window child = None;
    int rootX = 0;
    int rootY = 0;
    int windowX = 0;
    int windowY = 0;
    unsigned int mask = 0;
    if (!XQueryPointer(m_captureDisplay, m_window, &root, &child,
                       &rootX, &rootY, &windowX, &windowY, &mask))
        return;

    const glm::vec2 localPosition {
        static_cast<float>(windowX),
        static_cast<float>(windowY),
    };
    const glm::vec2 rootPosition {
        static_cast<float>(rootX),
        static_cast<float>(rootY),
    };
    const bool inside = localPosition.x >= 0.0f && localPosition.y >= 0.0f
                     && localPosition.x < m_windowSize.x
                     && localPosition.y < m_windowSize.y;
    m_pointerOutsideWindow = !inside;
    if (!inside)
        return;

    m_virtualCursor.reset(localPosition, rootPosition);
    handle_mouse_motion_event(localPosition.x, localPosition.y);
    Logger::debug("[Xlib] Synchronized overlay cursor after pointer capture");
}

void vkShade::InputBackendXlib::suspend_raw_mouse_input()
{
    int extensionOpcode = 0;
    int firstEvent = 0;
    int firstError = 0;
    if (!XQueryExtension(m_display, "XInputExtension",
                         &extensionOpcode, &firstEvent, &firstError))
        return;

    int maskCount = 0;
    XIEventMask* selected =
        XIGetSelectedEvents(m_display, DefaultRootWindow(m_display), &maskCount);
    if (!selected)
        return;

    m_savedXISelections.clear();
    m_savedXISelections.reserve(maskCount);

    for (int index = 0; index < maskCount; ++index)
    {
        std::vector<unsigned char> mask(
            selected[index].mask, selected[index].mask + selected[index].mask_len);
        auto hasEvent = [&mask](int eventType)
        {
            const size_t byte = static_cast<size_t>(eventType / 8);
            return byte < mask.size() && (mask[byte] & (1U << (eventType % 8))) != 0;
        };
        if (!hasEvent(XI_RawMotion)
            && !hasEvent(XI_RawButtonPress)
            && !hasEvent(XI_RawButtonRelease))
            continue;

        m_savedXISelections.push_back({selected[index].deviceid, mask});
        auto clearEvent = [&mask](int eventType)
        {
            const size_t byte = static_cast<size_t>(eventType / 8);
            if (byte < mask.size())
                mask[byte] &= static_cast<unsigned char>(~(1U << (eventType % 8)));
        };
        clearEvent(XI_RawMotion);
        clearEvent(XI_RawButtonPress);
        clearEvent(XI_RawButtonRelease);

        XIEventMask replacement {
            .deviceid = selected[index].deviceid,
            .mask_len = static_cast<int>(mask.size()),
            .mask = mask.data(),
        };
        XISelectEvents(m_display, DefaultRootWindow(m_display), &replacement, 1);
    }

    XFree(selected);
    XFlush(m_display);
}

void vkShade::InputBackendXlib::restore_raw_mouse_input()
{
    for (auto& selection : m_savedXISelections)
    {
        XIEventMask mask {
            .deviceid = selection.deviceId,
            .mask_len = static_cast<int>(selection.mask.size()),
            .mask = selection.mask.data(),
        };
        XISelectEvents(m_display, DefaultRootWindow(m_display), &mask, 1);
    }

    m_savedXISelections.clear();
    XFlush(m_display);
}

void vkShade::InputBackendXlib::handle_captured_event(const void* opaqueEvent)
{
    const auto& event = *static_cast<const XEvent*>(opaqueEvent);
    switch (event.type)
    {
        case MotionNotify:
        {
            const glm::vec2 rootPosition {
                static_cast<float>(event.xmotion.x_root),
                static_cast<float>(event.xmotion.y_root),
            };
            const glm::vec2 localPosition = rootPosition - m_windowRootPosition;
            const bool inside = localPosition.x >= 0.0f && localPosition.y >= 0.0f
                             && localPosition.x < m_windowSize.x
                             && localPosition.y < m_windowSize.y;
            if (!inside)
            {
                m_pointerOutsideWindow = true;
                m_virtualCursor.observe_root_motion(rootPosition);
                break;
            }

            if (m_pointerOutsideWindow)
            {
                m_pointerOutsideWindow = false;
                m_virtualCursor.reset(localPosition, rootPosition);
                handle_mouse_motion_event(localPosition.x, localPosition.y);
                break;
            }

            const auto position = m_virtualCursor.observe_root_motion(rootPosition);
            if (position)
                handle_mouse_motion_event(position->x, position->y);
            break;
        }
        case ButtonPress:
        case ButtonRelease:
        {
            if (event.type == ButtonPress
                && event.xbutton.button >= Button4 && event.xbutton.button <= 7)
            {
                if (event.xbutton.button <= Button5)
                {
                    handle_mouse_wheel_event(
                        0.0f, event.xbutton.button == Button4 ? 1.0f : -1.0f);
                }
                else
                {
                    handle_mouse_wheel_event(
                        event.xbutton.button == 6 ? 1.0f : -1.0f, 0.0f);
                }
                break;
            }

            MouseButton button;
            switch (event.xbutton.button)
            {
                case Button1: button = MouseButton::LEFT; break;
                case Button2: button = MouseButton::MIDDLE; break;
                case Button3: button = MouseButton::RIGHT; break;
                default: return;
            }
            handle_mouse_button_event(button, event.type == ButtonPress);
            break;
        }
    }
}

void vkShade::InputBackendXlib::process_events()
{
    if (!m_display || !m_xkbState)
        return;

    if (m_captureDisplay)
    {
        while (XPending(m_captureDisplay))
        {
            XEvent event {};
            XNextEvent(m_captureDisplay, &event);
            handle_captured_event(&event);
        }
        reconcile_focus();
    }

    // Query current keyboard state without consuming events
    char keymap[32];
    XQueryKeymap(m_display, keymap);

    // Check each key for state changes
    for (int keycode = 8; keycode < 256; keycode++)  // X11 keycodes start at 8
    {
        int byte = keycode / 8;
        int bit = keycode % 8;

        bool currently_pressed = (keymap[byte] & (1 << bit)) != 0;
        bool previously_pressed = (m_previousKeymap[byte] & (1 << bit)) != 0;

        if (currently_pressed != previously_pressed)
        {
            handle_key_event(keycode, currently_pressed);
        }
    }

    // Update previous state
    std::memcpy(m_previousKeymap, keymap, sizeof(keymap));

    if (m_wheelDisplay)
    {
        while (XPending(m_wheelDisplay))
        {
            XEvent event {};
            XNextEvent(m_wheelDisplay, &event);
            if (event.type != GenericEvent
                || event.xcookie.extension != m_wheelXiOpcode
                || event.xcookie.evtype != XI_RawButtonPress
                || !XGetEventData(m_wheelDisplay, &event.xcookie))
                continue;

            const auto* rawEvent = static_cast<XIRawEvent*>(event.xcookie.data);
            const int button = rawEvent->detail;
            XFreeEventData(m_wheelDisplay, &event.xcookie);

            if (button >= Button4 && button <= 7
                && is_window_active(m_wheelDisplay)
                && is_pointer_inside_window(m_wheelDisplay))
            {
                if (button <= Button5)
                {
                    handle_mouse_wheel_event(
                        0.0f, button == Button4 ? 1.0f : -1.0f);
                }
                else
                {
                    handle_mouse_wheel_event(
                        button == 6 ? 1.0f : -1.0f, 0.0f);
                }
            }
        }
    }

    if (!m_mouseCaptured)
        query_mouse_state();
}

Window vkShade::InputBackendXlib::get_top_level_window(Display* display, Window window)
{
    // Focus properties may identify a Wine child or window-manager frame, so comparisons use top-level identities.
    Window root = None;
    Window parent = None;
    Window* children = nullptr;
    unsigned int childCount = 0;
    Window current = window;

    while (XQueryTree(display, current, &root, &parent, &children, &childCount))
    {
        if (children)
            XFree(children);
        if (parent == None || parent == root)
            return current;
        current = parent;
    }
    return window;
}

bool vkShade::InputBackendXlib::is_window_active(Display* display)
{
    // Root-window XI2 events are shared across X11 clients, so forward them only while this surface owns focus.
    Window focusWindow = None;
    int revertTo = RevertToNone;
    XGetInputFocus(display, &focusWindow, &revertTo);
    if (focusWindow == None || focusWindow == PointerRoot
        || get_top_level_window(display, focusWindow) != m_topLevelWindow)
        return false;

    const Atom activeWindowAtom = XInternAtom(display, "_NET_ACTIVE_WINDOW", True);
    if (activeWindowAtom == None)
        return true;

    Atom actualType = None;
    int actualFormat = 0;
    unsigned long itemCount = 0;
    unsigned long remaining = 0;
    unsigned char* data = nullptr;
    const int result = XGetWindowProperty(
        display, DefaultRootWindow(display), activeWindowAtom,
        0, 1, False, XA_WINDOW, &actualType, &actualFormat,
        &itemCount, &remaining, &data);
    Window activeWindow = None;
    if (result == Success && actualType == XA_WINDOW && actualFormat == 32 && itemCount == 1)
        activeWindow = *reinterpret_cast<Window*>(data);
    if (data)
        XFree(data);

    return activeWindow == None
        || get_top_level_window(display, activeWindow) == m_topLevelWindow;
}

bool vkShade::InputBackendXlib::is_pointer_inside_window(Display* display)
{
    // Keyboard focus may remain here after the pointer leaves, but the global XI2 observer continues receiving events.
    XWindowAttributes attributes {};
    if (!XGetWindowAttributes(display, m_window, &attributes))
        return false;

    Window root = None;
    Window child = None;
    int rootX = 0;
    int rootY = 0;
    int windowX = 0;
    int windowY = 0;
    unsigned int mask = 0;
    if (!XQueryPointer(display, m_window, &root, &child,
                       &rootX, &rootY, &windowX, &windowY, &mask))
        return false;

    return windowX >= 0 && windowY >= 0
        && windowX < attributes.width && windowY < attributes.height;
}

void vkShade::InputBackendXlib::handle_key_event(uint32_t keyCode, bool pressed)
{
    if (!m_xkbState)
        return;

    // Update XKB state with this key event
    if (pressed)
        xkb_state_update_key(m_xkbState, keyCode, XKB_KEY_DOWN);
    else
        xkb_state_update_key(m_xkbState, keyCode, XKB_KEY_UP);

    // Call base class to update key state map
    this->handle_keyboard_event(keyCode, pressed);
}


void vkShade::InputBackendXlib::query_mouse_state()
{
    Window root_return, child_return;
    int root_x, root_y, win_x, win_y;
    unsigned int mask_return;

    // Query pointer position and button state
    Bool result = XQueryPointer(m_display, m_window,
                                &root_return, &child_return,
                                &root_x, &root_y,
                                &win_x, &win_y,
                                &mask_return);

    if (!result)
        return;  // Pointer not in our window

    // Current state
    glm::vec2 currentPos(static_cast<float>(win_x), static_cast<float>(win_y));
    bool left_pressed = (mask_return & Button1Mask) != 0;
    bool middle_pressed = (mask_return & Button2Mask) != 0;
    bool right_pressed = (mask_return & Button3Mask) != 0;

    // Only update if position changed
    if (currentPos != m_prevMousePos)
    {
        handle_mouse_motion_event(currentPos.x, currentPos.y);
        m_prevMousePos = currentPos;
    }

    // Only update if button state changed
    if (left_pressed != m_prevLeftButton)
    {
        handle_mouse_button_event(MouseButton::LEFT, left_pressed);
        m_prevLeftButton = left_pressed;
    }

    if (middle_pressed != m_prevMiddleButton)
    {
        handle_mouse_button_event(MouseButton::MIDDLE, middle_pressed);
        m_prevMiddleButton = middle_pressed;
    }

    if (right_pressed != m_prevRightButton)
    {
        handle_mouse_button_event(MouseButton::RIGHT, right_pressed);
        m_prevRightButton = right_pressed;
    }
}
