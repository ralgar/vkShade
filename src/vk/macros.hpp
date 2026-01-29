#pragma once

#include <magic_enum/magic_enum.hpp>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan.h>

// Abort immediately when there is an error.
#define VK_CHECK(x)                                                             \
{                                                                               \
    VkResult res = x;                                                           \
    if (res)                                                                    \
    {                                                                           \
        spdlog::critical("VkResult code: {}", magic_enum::enum_name(res));      \
        std::abort();                                                           \
    }                                                                           \
}
