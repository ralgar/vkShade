#include "input_backend_xlib.hpp"

#include <algorithm>
#include <cstring>

#include "core/logger.hpp"
#include "xinput_raw_motion.hpp"
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

    // A private XI2 subscription avoids exclusive core-event selection and
    // remains observable when a different X client owns the core pointer grab.
    m_xiObserverDisplay = XOpenDisplay(DisplayString(m_display));
    if (m_xiObserverDisplay)
    {
        m_topLevelWindow = get_top_level_window(m_xiObserverDisplay, m_window);
        int event = 0;
        int error = 0;
        int major = 2;
        int minor = 0;
        const bool xi2Available = XQueryExtension(m_xiObserverDisplay, "XInputExtension",
            &m_xiObserverOpcode, &event, &error) && XIQueryVersion(m_xiObserverDisplay, &major, &minor) == Success;

        if (xi2Available)
        {
            unsigned char mask[XIMaskLen(XI_LASTEVENT)] {};
            XISetMask(mask, XI_RawMotion);
            XISetMask(mask, XI_RawButtonPress);
            XISetMask(mask, XI_RawButtonRelease);
            XIEventMask eventMask {
                .deviceid = XIAllDevices,
                .mask_len = static_cast<int>(sizeof(mask)),
                .mask = mask,
            };
            XISelectEvents(m_xiObserverDisplay, DefaultRootWindow(m_xiObserverDisplay), &eventMask, 1);
            XFlush(m_xiObserverDisplay);
        }
        else
        {
            Logger::warn("[Xlib] XInput2 is unavailable for mouse input observation");
            XCloseDisplay(m_xiObserverDisplay);
            m_xiObserverDisplay = nullptr;
        }
    }
    else
    {
        Logger::warn("[Xlib] Could not open a display connection for mouse input observation");
    }

    initialize_mouse_capture(*this);
}

vkShade::InputBackendXlib::~InputBackendXlib()
{
    shutdown_mouse_capture();
    if (m_xiObserverDisplay)
        XCloseDisplay(m_xiObserverDisplay);
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

    if (!m_captureRequested)
    {
        m_captureRequested = true;
        initialize_virtual_cursor();
    }

    m_mouseCaptured = begin_mouse_capture();
    if (!m_mouseCaptured)
        return {MouseCaptureStatus::Inactive, true};

    Logger::debug("[Xlib] Mouse input captured for overlay");
    return {get_status(), false};
}

void vkShade::InputBackendXlib::release()
{
    m_captureRequested = false;
    m_lastGrabFailure.reset();
    if (!m_mouseCaptured && !m_captureDisplay)
        return;

    end_mouse_capture();
    m_mouseCaptured = false;
    Logger::debug("[Xlib] Mouse input returned to application");
}

void vkShade::InputBackendXlib::initialize_virtual_cursor()
{
    XWindowAttributes attributes {};
    m_applicationPointerEventMask = 0;
    if (!XGetWindowAttributes(m_display, m_window, &attributes))
        return;

    m_applicationPointerEventMask =
        static_cast<unsigned int>(attributes.your_event_mask)
        & ALL_POINTER_EVENT_MASKS;

    Window child = None;
    int rootX = 0;
    int rootY = 0;
    XTranslateCoordinates(m_display, m_window, DefaultRootWindow(m_display),
                          0, 0, &rootX, &rootY, &child);

    m_windowSize = {
        static_cast<float>(attributes.width),
        static_cast<float>(attributes.height),
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
    const glm::vec2 warpPosition = m_windowRootPosition + center;
    m_virtualCursor.set_bounds(bounds);
    glm::vec2 initialPosition = center;
    glm::vec2 initialRootPosition = warpPosition;
    Window pointerRoot = None;
    Window pointerChild = None;
    int pointerRootX = 0;
    int pointerRootY = 0;
    int pointerWindowX = 0;
    int pointerWindowY = 0;
    unsigned int pointerMask = 0;
    if (XQueryPointer(
            m_display, m_window, &pointerRoot, &pointerChild,
            &pointerRootX, &pointerRootY, &pointerWindowX, &pointerWindowY,
            &pointerMask)
        && pointerWindowX >= 0 && pointerWindowY >= 0
        && pointerWindowX < attributes.width && pointerWindowY < attributes.height)
    {
        initialPosition = {
            static_cast<float>(pointerWindowX),
            static_cast<float>(pointerWindowY),
        };
        initialRootPosition = {
            static_cast<float>(pointerRootX),
            static_cast<float>(pointerRootY),
        };
    }
    m_virtualCursor.reset(initialPosition, initialRootPosition);
    m_virtualCursor.set_warp_position(warpPosition);
    handle_mouse_motion_event(initialPosition.x, initialPosition.y);
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
        if (!m_lastGrabFailure || *m_lastGrabFailure != grabStatus)
        {
            Logger::warn(
                "[Xlib] Could not capture pointer for overlay (grab status {})", grabStatus);
        }
        else
        {
            Logger::trace(
                "[Xlib] Pointer capture remains pending (grab status {})", grabStatus);
        }
        m_lastGrabFailure = grabStatus;
        XFreeCursor(m_captureDisplay, m_hiddenCursor);
        XDestroyWindow(m_captureDisplay, m_captureWindow);
        XCloseDisplay(m_captureDisplay);
        m_hiddenCursor = 0;
        m_captureWindow = 0;
        m_captureDisplay = nullptr;
        restore_pointer_grab();
        return false;
    }

    m_lastGrabFailure.reset();
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

    const Atom activeWindowAtom =
        XInternAtom(display, "_NET_ACTIVE_WINDOW", True);
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

    Logger::trace("[Xlib] Releasing application pointer grab for overlay");
    // A grab owned by the application's X client can only be released through
    // its borrowed display. Remember enough state to restore it afterwards;
    // a foreign-client grab remains untouched and falls back to XI2 observation.
    if (!m_restorePointerGrab)
    {
        m_restorePointerGrabEventMask = m_applicationPointerEventMask
            ? m_applicationPointerEventMask
            : OVERLAY_POINTER_EVENT_MASK;
        m_restorePointerGrab = true;
    }

    XUngrabPointer(m_display, CurrentTime);
    XSync(m_display, False);
    result = grab_pointer();
    if (result == AlreadyGrabbed)
    {
        m_restorePointerGrab = false;
        m_restorePointerGrabEventMask = 0;
    }
    return result;
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
    if (m_display)
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
                const auto position = m_virtualCursor.observe_root_motion(
                    rootPosition, localPosition);
                if (position)
                    handle_mouse_motion_event(position->x, position->y);
                break;
            }

            if (m_pointerOutsideWindow)
            {
                m_pointerOutsideWindow = false;
                m_virtualCursor.reset(localPosition, rootPosition);
                handle_mouse_motion_event(localPosition.x, localPosition.y);
                break;
            }

            const auto position = m_virtualCursor.observe_root_motion(
                rootPosition, localPosition);
            if (position)
                handle_mouse_motion_event(position->x, position->y);
            break;
        }
        case ButtonPress:
        case ButtonRelease:
        {
            if (event.type == ButtonPress
                && forward_x11_wheel_button(event.xbutton.button))
                break;

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

bool vkShade::InputBackendXlib::forward_x11_wheel_button(int button)
{
    if (button == Button4 || button == Button5)
    {
        handle_mouse_wheel_event(0.0f, button == Button4 ? 1.0f : -1.0f);
        return true;
    }
    if (button == 6 || button == 7)
    {
        handle_mouse_wheel_event(button == 6 ? 1.0f : -1.0f, 0.0f);
        return true;
    }
    return false;
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

    if (m_xiObserverDisplay)
    {
        const bool hasEvents = XPending(m_xiObserverDisplay) > 0;
        const bool windowActive = hasEvents && is_window_active(m_xiObserverDisplay);
        const bool rawFallbackActive =
            windowActive && m_captureRequested && !m_pointerGrabbed;
        const bool wheelObserverActive =
            windowActive && !m_captureRequested
            && is_pointer_inside_window(m_xiObserverDisplay);

        while (XPending(m_xiObserverDisplay))
        {
            XEvent event {};
            XNextEvent(m_xiObserverDisplay, &event);
            if (event.type != GenericEvent
                || event.xcookie.extension != m_xiObserverOpcode
                || !XGetEventData(m_xiObserverDisplay, &event.xcookie))
                continue;

            const auto* rawEvent = static_cast<XIRawEvent*>(event.xcookie.data);
            const int eventType = event.xcookie.evtype;
            const int button = rawEvent->detail;
            // XI2 emits paired master and physical-source events. Forward only
            // the source event so one physical action produces one GUI event.
            const bool sourceEvent = rawEvent->deviceid == rawEvent->sourceid;
            glm::vec2 motion {0.0f, 0.0f};
            if (eventType == XI_RawMotion && rawFallbackActive && sourceEvent)
            {
                motion = decode_xinput_relative_motion(
                    std::span(
                        rawEvent->valuators.mask,
                        static_cast<size_t>(rawEvent->valuators.mask_len)),
                    rawEvent->valuators.values);
            }
            XFreeEventData(m_xiObserverDisplay, &event.xcookie);

            if (eventType == XI_RawMotion && rawFallbackActive
                && sourceEvent && motion != glm::vec2 {0.0f, 0.0f})
            {
                const glm::vec2 position =
                    m_virtualCursor.observe_relative_motion(motion);
                handle_mouse_motion_event(position.x, position.y);
                continue;
            }

            const bool pressed = eventType == XI_RawButtonPress;
            const bool released = eventType == XI_RawButtonRelease;
            if (!pressed && !released)
                continue;

            if (sourceEvent && rawFallbackActive
                && button >= Button1 && button <= Button3)
            {
                const MouseButton mouseButton = button == Button1
                    ? MouseButton::LEFT
                    : button == Button2 ? MouseButton::MIDDLE : MouseButton::RIGHT;
                handle_mouse_button_event(mouseButton, pressed);
                continue;
            }

            if (should_forward_xinput_wheel(
                    button, sourceEvent, pressed,
                    rawFallbackActive, wheelObserverActive))
                forward_x11_wheel_button(button);
        }

    }

    // Absolute polling observes application center-warps. During capture the
    // private grab or XI2 fallback owns the virtual cursor instead.
    if (!m_captureRequested)
        query_mouse_state();
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
