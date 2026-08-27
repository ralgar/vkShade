#pragma once

#include <filesystem>
#include <memory>

#include <glm/glm.hpp>
#include <vulkan/vulkan_core.h>

#include "core/logger.hpp"
#include "core/uniform.hpp"
#include "hooks/hooks.hpp"
#include "vk/buffer.hpp"
#include "vk/reshade_uniforms.hpp"
#include "vk/shader_module.hpp"

namespace reshadefx
{
    enum class blend_factor : uint8_t;
    enum class blend_op : uint8_t;
    enum class primitive_topology : uint8_t;
    enum class stencil_func : uint8_t;
    enum class stencil_op : uint8_t;
    struct effect_module;
    struct type;
    struct uniform;
}

namespace vkShade
{
    class VulkanImage;
    class VulkanSampler;

    class ReshadeEffect : public VulkanObject
    {
    public:
        ReshadeEffect(VulkanDevice& device, VkExtent2D extent, VkFormat format, std::filesystem::path effectPath);
        ~ReshadeEffect() override;

        void apply(VkCommandBuffer cmd, VulkanImage& outputImage);
        void bind_input(VulkanImage& colorImage, VulkanImage& depthImage);
        void update(const ReshadeFrameState& frame);

    private:
        struct Pass
        {
            VkPipeline            pipeline       {VK_NULL_HANDLE};
            VkPipelineLayout      pipelineLayout {VK_NULL_HANDLE};
            VkDescriptorSetLayout imageSetLayout {VK_NULL_HANDLE};
            VkDescriptorSet       imageSet       {VK_NULL_HANDLE};

            std::shared_ptr<ShaderModule> vertexShader;
            std::shared_ptr<ShaderModule> fragmentShader;
        };

        VkExtent2D  m_extent;
        VkFormat    m_format;
        std::string m_fileName;

        std::unique_ptr<reshadefx::effect_module> m_module {nullptr};
        std::unique_ptr<VulkanBuffer> m_uniformBuffer {nullptr};
        std::unordered_map<std::string, Uniform> m_uniformsByName;
        std::vector<std::unique_ptr<ReshadeUniform>> m_builtinUniforms;

        VkDescriptorPool m_descriptorPool;
        VkDescriptorSetLayout m_uniformSetLayout;
        VkDescriptorSet m_uniformSet;

        std::unordered_map<std::string, std::unique_ptr<VulkanImage>> m_textures;
        std::vector<std::unique_ptr<VulkanSampler>> m_samplers;
        std::unique_ptr<VulkanImage> m_stencilBuffer;
        bool m_imagesInitialized = false;

        std::vector<Pass> m_passes;

        bool compile(VkExtent2D extent, std::filesystem::path filePath);

        Uniform* find_uniform(const std::string& name)
        {
            auto it = m_uniformsByName.find(name);
            if (it != m_uniformsByName.end())
            {
                return &it->second;
            }
            else
            {
                return nullptr;
            }
        }

        template<typename T>
        void on_uniform_changed(const std::string& name, T value)
        {
            // Search for the uniform by name
            const Uniform* uniform = nullptr;
            auto it = m_uniformsByName.find(name);
            if (it == m_uniformsByName.end())
            {
                Logger::error("Uniform '{}' not found", name);
                return;
            }
            uniform = &it->second;

            // Make sure the type is correct
            if (uniform->type != uniform_type_v<T>)
            {
                Logger::error("Type mismatch for uniform '{}'", name);
                return;
            }

            // Make sure the buffer is good
            if (!m_uniformBuffer)
            {
                Logger::error("Uniform buffer is not valid: {}", m_fileName);
                return;
            }

            // Write T into the buffer at the specified offset
            if (!m_uniformBuffer->write(&value, sizeof(T), uniform->offset))
            {
                Logger::error("Failed to write uniform '{}'", name);
                return;
            }
        }

        void reflect_descriptors();
        void reflect_images();
        void reflect_pipeline();
        void reflect_samplers();
        void reflect_uniforms();

        static VkBlendFactor       convert_blend_factor(reshadefx::blend_factor blendFactor);
        static VkBlendOp           convert_blend_op(reshadefx::blend_op blendOp);
        static VkPrimitiveTopology convert_primitive_topology(reshadefx::primitive_topology topology);
        static VkCompareOp         convert_stencil_func(reshadefx::stencil_func stencilFunc);
        static VkStencilOp         convert_stencil_op(reshadefx::stencil_op stencilOp);
        static Uniform::Type       convert_uniform_type(reshadefx::type type);
    };
} // namespace vkShade
