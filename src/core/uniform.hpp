#pragma once

#include <cstdint>
#include <string>

namespace vkShade
{
    struct Uniform
    {
        enum class Type
        {
            Unknown = 0,
            Float, Vec2, Vec3, Vec4,
        };

        std::string name;
        uint32_t size {0};
        uint32_t offset {0};
        Type type {Type::Unknown};
    };
} // namespace vkShade
