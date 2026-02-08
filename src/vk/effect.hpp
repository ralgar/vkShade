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
        Effect(VulkanDevice& device, VkFormat outputFormat, const std::string& fileName);
        ~Effect();

        void bind_input(VkImageView inputView);
        void render(VkCommandBuffer cmd, VkExtent2D extent);

    private:
        VkPipeline m_pipeline;
        VkPipelineLayout m_pipelineLayout;
        VkDescriptorPool m_descriptorPool;
        VkDescriptorSet m_descriptorSet;
        VkDescriptorSetLayout m_descriptorSetLayout;
        VkSampler m_sampler;

        // Shaders
        std::shared_ptr<vkShade::ShaderModule> m_vertShader;
        std::shared_ptr<vkShade::ShaderModule> m_fragShader;

        void create_descriptor_pool();
        void allocate_descriptor_set();
    };
} // namespace vkShade
