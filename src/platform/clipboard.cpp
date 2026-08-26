#include "clipboard.hpp"

#if defined(__linux__)
    #include "linux/xcb_clipboard.hpp"
    using ClipboardImpl = vkShade::Platform::XcbClipboard;
#else
    #error "Unsupported platform"
#endif

#include "core/logger.hpp"

namespace vkShade::Platform
{
    std::unique_ptr<Clipboard> Clipboard::create()
    {
        auto clipboard = std::make_unique<ClipboardImpl>();
        if (!clipboard->available())
        {
            // TODO: Add a native Wayland data-device provider for sessions
            // without an X11/Xwayland DISPLAY.
            Logger::debug("System clipboard is unavailable");
            return nullptr;
        }

        Logger::debug("Initialized X11 system clipboard integration");
        return clipboard;
    }
} // namespace vkShade::Platform
