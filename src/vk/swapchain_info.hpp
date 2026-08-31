#pragma once

#include <vulkan/vulkan.h>

namespace vkShade
{
    struct SwapchainInfo
    {
        VkExtent2D      extent;
        VkFormat        format;
        VkColorSpaceKHR colorSpace;
    };
} // namespace vkShade
