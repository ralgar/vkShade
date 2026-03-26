#pragma once

#include <filesystem>
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
        ~Effect() override;

        void apply(VkCommandBuffer cmd, VkExtent2D extent);
        void bind_input(VkImageView inputView);

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

        void allocate_descriptor_set();
        void create_descriptor_pool();

        bool compile_reshadefx(std::filesystem::path filePath);
    };
} // namespace vkShade
