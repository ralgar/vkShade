#pragma once

#include <vulkan/vulkan_core.h>

namespace vkShade
{
    // Common base class for Vulkan resource wrappers.
    // All Vulkan resources will need a handle to the device.
    class VulkanObject
    {
    protected:
        VkDevice m_device;
    };
}  // namespace vkShade
