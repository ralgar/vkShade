#pragma once

#include <string>

#include "vulkan_object.hpp"

namespace vkShade
{
    class VulkanShaderModule : public VulkanObject
    {
    public:
        VulkanShaderModule(VkDevice device, const std::string& filePath);
        ~VulkanShaderModule();

        VkShaderModule module() const { return m_module; }

    private:
        VkShaderModule m_module {VK_NULL_HANDLE};
    };
} // namespace vkShade
