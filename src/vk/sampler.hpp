#pragma once

#include "vk/object.hpp"

namespace reshadefx
{
    struct sampler;
    enum class filter_mode : uint8_t;
    enum class texture_address_mode : uint8_t;
}

namespace vkShade
{
    class VulkanSampler : public VulkanObject
    {
    public:
        VulkanSampler(VulkanDevice& device, const reshadefx::sampler& samplerInfo);
        ~VulkanSampler();

        VkSampler handle() const { return m_sampler; }

    private:
        VkSampler m_sampler {VK_NULL_HANDLE};

        // Convert from ReShade types to Vulkan types
        static VkSamplerAddressMode convert_address_mode(reshadefx::texture_address_mode addressMode);
        static VkFilter             convert_mag_filter(reshadefx::filter_mode filterMode);
        static VkFilter             convert_min_filter(reshadefx::filter_mode filterMode);
        static VkSamplerMipmapMode  convert_mipmap_mode(reshadefx::filter_mode filterMode);
    };
} // namespace vkShade
