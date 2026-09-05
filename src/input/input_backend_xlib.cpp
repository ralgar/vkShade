#include "input_backend_xlib.hpp"

#include <cstring>

#include "core/logger.hpp"
#include <X11/extensions/XInput2.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <xkbcommon/xkbcommon.h>

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
}

vkShade::InputBackendXlib::~InputBackendXlib()
{
    if (m_wheelDisplay)
        XCloseDisplay(m_wheelDisplay);
}

void vkShade::InputBackendXlib::process_events()
{
    if (!m_display || !m_xkbState)
        return;

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

    // Query mouse state
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
