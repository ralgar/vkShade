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

        // Non-copyable, movable.
        VulkanObject(const VulkanObject&) = delete;
        VulkanObject& operator=(const VulkanObject&) = delete;
        VulkanObject(VulkanObject&&) noexcept = default;
        VulkanObject& operator=(VulkanObject&&) noexcept = default;

        VulkanDevice& device() const { return m_device; }

    protected:
        VulkanObject(VulkanDevice& device) : m_device(device) {}

        VulkanDevice& m_device;
    };
}  // namespace vkShade
