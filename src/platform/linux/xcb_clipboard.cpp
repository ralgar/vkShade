#include "xcb_clipboard.hpp"

#include <array>
#include <cerrno>
#include <cstdlib>
#include <poll.h>

namespace vkShade::Platform
{
    XcbClipboard::XcbClipboard()
    {
        int screenNumber = 0;
        m_connection = xcb_connect(nullptr, &screenNumber);
        if (!m_connection || xcb_connection_has_error(m_connection))
            return;

        const xcb_setup_t* setup = xcb_get_setup(m_connection);
        xcb_screen_iterator_t screens = xcb_setup_roots_iterator(setup);
        for (int index = 0; index < screenNumber && screens.rem; ++index)
            xcb_screen_next(&screens);
        if (!screens.data)
            return;

        m_window = xcb_generate_id(m_connection);
        xcb_create_window(
            m_connection, XCB_COPY_FROM_PARENT, m_window, screens.data->root,
            0, 0, 1, 1, 0, XCB_WINDOW_CLASS_INPUT_ONLY,
            XCB_COPY_FROM_PARENT, 0, nullptr);

        m_clipboardAtom = intern_atom("CLIPBOARD");
        m_targetsAtom = intern_atom("TARGETS");
        m_textAtom = intern_atom("TEXT");
        m_utf8StringAtom = intern_atom("UTF8_STRING");
        if (!available())
            return;

        xcb_flush(m_connection);
        m_eventThread = std::jthread(
            [this](std::stop_token stopToken) { process_events(stopToken); });
    }

    XcbClipboard::~XcbClipboard()
    {
        m_eventThread.request_stop();
        if (m_eventThread.joinable())
            m_eventThread.join();

        if (m_connection)
        {
            if (m_window != XCB_WINDOW_NONE)
                xcb_destroy_window(m_connection, m_window);
            xcb_disconnect(m_connection);
        }
    }

    bool XcbClipboard::available() const
    {
        return m_connection && !xcb_connection_has_error(m_connection) &&
               m_window != XCB_WINDOW_NONE &&
               m_clipboardAtom != XCB_ATOM_NONE &&
               m_targetsAtom != XCB_ATOM_NONE &&
               m_textAtom != XCB_ATOM_NONE &&
               m_utf8StringAtom != XCB_ATOM_NONE;
    }

    bool XcbClipboard::set_text(std::string_view text)
    {
        if (!available())
            return false;

        {
            const std::scoped_lock lock(m_textMutex);
            m_text.assign(text);
        }

        xcb_set_selection_owner(
            m_connection, m_window, m_clipboardAtom, XCB_CURRENT_TIME);
        return xcb_flush(m_connection) > 0;
    }

    xcb_atom_t XcbClipboard::intern_atom(const char* name)
    {
        const xcb_intern_atom_cookie_t cookie =
            xcb_intern_atom(m_connection, 0, std::char_traits<char>::length(name), name);
        xcb_intern_atom_reply_t* reply =
            xcb_intern_atom_reply(m_connection, cookie, nullptr);
        if (!reply)
            return XCB_ATOM_NONE;

        const xcb_atom_t atom = reply->atom;
        std::free(reply);
        return atom;
    }

    void XcbClipboard::process_events(std::stop_token stopToken)
    {
        pollfd descriptor {
            .fd = xcb_get_file_descriptor(m_connection),
            .events = POLLIN,
            .revents = 0,
        };

        while (!stopToken.stop_requested())
        {
            const int result = poll(&descriptor, 1, 100);
            if (result < 0)
            {
                if (errno == EINTR)
                    continue;
                break;
            }
            if (result == 0)
                continue;

            while (xcb_generic_event_t* event = xcb_poll_for_event(m_connection))
            {
                const uint8_t type = event->response_type & ~0x80;
                if (type == XCB_SELECTION_REQUEST)
                {
                    handle_selection_request(
                        *reinterpret_cast<xcb_selection_request_event_t*>(event));
                }
                std::free(event);
            }

            if (xcb_connection_has_error(m_connection))
                break;
        }
    }

    void XcbClipboard::handle_selection_request(
        const xcb_selection_request_event_t& request)
    {
        xcb_selection_notify_event_t notification {
            .response_type = XCB_SELECTION_NOTIFY,
            .sequence = 0,
            .time = request.time,
            .requestor = request.requestor,
            .selection = request.selection,
            .target = request.target,
            .property = XCB_ATOM_NONE,
        };

        const xcb_atom_t property = request.property != XCB_ATOM_NONE
                                  ? request.property
                                  : request.target;

        if (request.target == m_targetsAtom)
        {
            const std::array<xcb_atom_t, 4> targets = {
                m_targetsAtom,
                m_utf8StringAtom,
                m_textAtom,
                static_cast<xcb_atom_t>(XCB_ATOM_STRING),
            };
            xcb_change_property(
                m_connection, XCB_PROP_MODE_REPLACE, request.requestor,
                property, XCB_ATOM_ATOM, 32, targets.size(), targets.data());
            notification.property = property;
        }
        else if (request.target == m_utf8StringAtom ||
                 request.target == m_textAtom ||
                 request.target == XCB_ATOM_STRING)
        {
            std::string text;
            {
                const std::scoped_lock lock(m_textMutex);
                text = m_text;
            }
            xcb_change_property(
                m_connection, XCB_PROP_MODE_REPLACE, request.requestor,
                property, request.target, 8, text.size(), text.data());
            notification.property = property;
        }

        xcb_send_event(
            m_connection, 0, request.requestor, XCB_EVENT_MASK_NO_EVENT,
            reinterpret_cast<const char*>(&notification));
        xcb_flush(m_connection);
    }
} // namespace vkShade::Platform
