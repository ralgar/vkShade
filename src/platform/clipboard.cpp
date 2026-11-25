#include "clipboard.hpp"

#include <utility>

#if defined(__linux__)
    #include "linux/wayland_clipboard.hpp"
    #include "linux/xcb_clipboard.hpp"
#else
    #error "Unsupported platform"
#endif

#include "core/logger.hpp"

namespace vkShade::Platform
{
    std::unique_ptr<Clipboard> Clipboard::create(
        std::shared_ptr<WaylandClientState> waylandState)
    {
        if (waylandState)
        {
            auto clipboard = std::make_unique<WaylandClipboard>(std::move(waylandState));
            if (clipboard->is_available())
            {
                Logger::debug("Initialized Wayland system clipboard integration");
                return clipboard;
            }
        }

        auto clipboard = std::make_unique<XcbClipboard>();
        if (clipboard->is_available())
        {
            Logger::debug("Initialized X11 system clipboard integration");
            return clipboard;
        }

        Logger::debug("System clipboard is unavailable");
        return nullptr;
    }
} // namespace vkShade::Platform
