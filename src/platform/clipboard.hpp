#pragma once

#include <memory>
#include <string_view>

namespace vkShade::Platform
{
    class Clipboard
    {
    public:
        virtual ~Clipboard() = default;

        virtual bool set_text(std::string_view text) = 0;

        static std::unique_ptr<Clipboard> create();
    };
} // namespace vkShade::Platform
