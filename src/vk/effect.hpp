#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>

#include <vulkan/vulkan_core.h>

#include "hooks/hooks.hpp"
#include "vk/buffer.hpp"
#include "vk/shader_module.hpp"

namespace reshadefx
{
    struct effect_module;
}

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
        std::unique_ptr<reshadefx::effect_module> m_module {nullptr};
        std::unique_ptr<VulkanBuffer> m_uniformBuffer {nullptr};

        VkPipeline m_pipeline;
        VkPipelineLayout m_pipelineLayout;
        VkDescriptorPool m_descriptorPool;
        VkDescriptorSet m_uniformSet;
        VkDescriptorSet m_imageSet;
        VkDescriptorSetLayout m_uniformSetLayout;
        VkDescriptorSetLayout m_imageSetLayout;
        VkSampler m_sampler;

        // Shaders
        std::shared_ptr<vkShade::ShaderModule> m_vertShader;
        std::shared_ptr<vkShade::ShaderModule> m_fragShader;

        void create_descriptor_sets();
        void create_pipeline(VkFormat outputFormat);

        bool compile(std::filesystem::path filePath);
    };
} // namespace vkShade
