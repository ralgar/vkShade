#pragma once

#include "platform/clipboard.hpp"

#include <mutex>
#include <string>
#include <thread>

#include <xcb/xcb.h>

namespace vkShade::Platform
{
    class XcbClipboard final : public Clipboard
    {
    public:
        XcbClipboard();
        ~XcbClipboard() override;

        bool available() const;
        bool set_text(std::string_view text) override;

    private:
        xcb_atom_t intern_atom(const char* name);
        void process_events(std::stop_token stopToken);
        void handle_selection_request(const xcb_selection_request_event_t& request);

        xcb_connection_t* m_connection = nullptr;
        xcb_window_t m_window = XCB_WINDOW_NONE;
        xcb_atom_t m_clipboardAtom = XCB_ATOM_NONE;
        xcb_atom_t m_targetsAtom = XCB_ATOM_NONE;
        xcb_atom_t m_textAtom = XCB_ATOM_NONE;
        xcb_atom_t m_utf8StringAtom = XCB_ATOM_NONE;

        std::mutex m_textMutex;
        std::string m_text;
        std::jthread m_eventThread;
    };
} // namespace vkShade::Platform
