#pragma once

#include <memory>
#include <string_view>

namespace vkShade::Platform
{
    class WaylandClientState;

    class Clipboard
    {
    public:
        virtual ~Clipboard() = default;

        virtual bool set_text(std::string_view text) = 0;
        virtual void update() {}

        static std::unique_ptr<Clipboard> create(
            std::shared_ptr<WaylandClientState> waylandState = {});
    };
} // namespace vkShade::Platform
