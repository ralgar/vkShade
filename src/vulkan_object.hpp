#pragma once

#include <vulkan/vulkan_core.h>

namespace vkShade
{
    // Common base class for Vulkan resource wrappers.
    // All Vulkan resources will need a handle to the device.
    class VulkanObject
    {
    public:
        VkDevice device() const { return m_device; }

    protected:
        VkDevice m_device;
    };
}  // namespace vkShade
