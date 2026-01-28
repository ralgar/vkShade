#pragma once

#include <vulkan/vulkan_core.h>

namespace vkShade
{
    // Common base class for Vulkan resource wrappers.
    // All Vulkan resources will need a handle to the device.
    class VulkanObject
    {
    public:
        virtual ~VulkanObject() = default;

        // Non-copyable, movable.
        VulkanObject(const VulkanObject&) = delete;
        VulkanObject& operator=(const VulkanObject&) = delete;
        VulkanObject(VulkanObject&&) noexcept = default;
        VulkanObject& operator=(VulkanObject&&) noexcept = default;

        VkDevice device() const { return m_device; }

    protected:
        VulkanObject(VkDevice device) : m_device(device) {}

        VkDevice m_device {VK_NULL_HANDLE};
    };
}  // namespace vkShade
