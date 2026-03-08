#pragma once

#include <string>

namespace vkShade
{
    struct ConfigOption
    {
        std::string section;
        std::string name;
        std::string value;
    };
} // namespace vkShade
