#pragma once

#include "hooks/hooks.hpp"

namespace vkShade
{
    // Common base class for Vulkan resource wrappers.
    // All Vulkan resources will need a handle to the device.
    class VulkanObject
    {
    public:
        virtual ~VulkanObject() = default;

        // Non-copyable, non-movable.
        VulkanObject(const VulkanObject&) = delete;
        VulkanObject& operator=(const VulkanObject&) = delete;
        VulkanObject(VulkanObject&&) = delete;
        VulkanObject& operator=(VulkanObject&&) = delete;

    protected:
        explicit VulkanObject(VulkanDevice& device) : m_device(device) {}

        VulkanDevice& m_device;
    };
}  // namespace vkShade
