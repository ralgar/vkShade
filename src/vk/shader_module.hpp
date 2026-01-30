#pragma once

#include <string>

#include "core/resource.hpp"
#include "object.hpp"

namespace vkShade
{
    class ShaderModule : public Resource, VulkanObject
    {
    public:
        ShaderModule(VulkanDevice& device, const std::string& filePath);
        ~ShaderModule();

        bool load() override;

        VkShaderModule module() const { return m_module; }

    private:
        VkShaderModule m_module {VK_NULL_HANDLE};
        std::string m_filePath;
    };
} // namespace vkShade
