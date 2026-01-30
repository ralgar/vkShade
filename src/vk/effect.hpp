#pragma once

#include <memory>

#include <vulkan/vulkan_core.h>

#include "hooks/hooks.hpp"
#include "vk/shader_module.hpp"

namespace vkShade
{
    class Effect : public VulkanObject
    {
    public:
        Effect(VulkanDevice& device, VkFormat outputFormat);
        ~Effect();

        void render(VkCommandBuffer cmd, VkImageView input, VkExtent2D extent);

    private:
        VkPipeline m_pipeline;
        VkPipelineLayout m_pipelineLayout;
        VkDescriptorSetLayout m_descriptorSetLayout;
        VkDescriptorSet m_descriptorSet;

        // Shaders
        std::shared_ptr<vkShade::ShaderModule> m_vertShader;
        std::shared_ptr<vkShade::ShaderModule> m_fragShader;
    };
} // namespace vkShade
