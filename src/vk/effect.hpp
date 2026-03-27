#pragma once

#include <expected>
#include <filesystem>
#include <memory>

#include <glm/glm.hpp>
#include <vulkan/vulkan_core.h>

#include "core/uniform.hpp"
#include "hooks/hooks.hpp"
#include "vk/buffer.hpp"
#include "vk/shader_module.hpp"

namespace reshadefx
{
    struct effect_module;
    struct uniform;
}

namespace vkShade
{
    class Effect : public VulkanObject
    {
    public:
        Effect(VulkanDevice& device, VkExtent2D extent, VkFormat format, const std::string& fileName);
        ~Effect() override;

        enum class Error
        {
            None = 0,
            BufferNotValid,
            UniformNotFound,
            TypeMismatch,
            WriteError,
        };

        void apply(VkCommandBuffer cmd, VkExtent2D extent);
        void bind_input(VkImageView inputView);

        template <class T>
        inline std::expected<T, Error> get_uniform(const std::string& name)
        {
            // Search for a binding with the name passed into the func
            const Uniform* binding = this->find_uniform(name);
            if (!binding)
                return std::unexpected(Error::UniformNotFound);

            // Make sure the type is correct
            if (!matches_type<T>(binding->type))
                return std::unexpected(Error::TypeMismatch);

            // Make sure the buffer is good
            if (!m_uniformBuffer)
                return std::unexpected(Error::BufferNotValid);

            std::byte* dataStartingPoint = static_cast<std::byte*>(m_uniformBuffer->data()) + binding->offset;

            // Important to reinterpret cast using T, because some stuff might be 16byte-aligned and we want to only get
            // the bytes that correspond to the actual value type
            return *reinterpret_cast<T*>(dataStartingPoint);
        }

        template <typename T>
        inline Error set_uniform(const std::string& name, T value)
        {
            // Search for the uniform by name
            const Uniform* uniform = this->find_uniform(name);
            if (!uniform)
                return Error::UniformNotFound;

            // Make sure the type is correct
            if (!matches_type<T>(uniform->type))
                return Error::TypeMismatch;

            // Make sure the buffer is good
            if (!m_uniformBuffer)
                return Error::BufferNotValid;

            // Write T into the buffer at the specified offset
            if (!m_uniformBuffer->write(&value, sizeof(T), uniform->offset))
                return Error::WriteError;

            return Error::None;
        }

    private:
        std::unique_ptr<reshadefx::effect_module> m_module {nullptr};
        std::unique_ptr<VulkanBuffer> m_uniformBuffer {nullptr};
        std::unordered_map<std::string, Uniform> m_uniformsByName;

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
        inline bool matches_type(const Uniform::Type& type)
        {
            // Float types
            if constexpr (std::is_same_v<T, float>)        return type == Uniform::Type::Float;
            if constexpr (std::is_same_v<T, glm::vec2>)    return type == Uniform::Type::Vec2;
            if constexpr (std::is_same_v<T, glm::vec3>)    return type == Uniform::Type::Vec3;
            if constexpr (std::is_same_v<T, glm::vec4>)    return type == Uniform::Type::Vec4;

            return false;
        }

        void reflect_uniforms();
    };
} // namespace vkShade
