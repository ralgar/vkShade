#include "sampler.hpp"

#include <utility>

#include <effect_module.hpp>
#include "core/logger.hpp"

#include "macros.hpp"

vkShade::VulkanSampler::VulkanSampler(VulkanDevice& device, const reshadefx::sampler& samplerInfo)
    : VulkanObject(device)
{
    Logger::trace("Creating sampler");

    VkSamplerCreateInfo createInfo {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .pNext = nullptr,
        .magFilter  = convert_mag_filter(samplerInfo.filter),
        .minFilter  = convert_min_filter(samplerInfo.filter),
        .mipmapMode = convert_mipmap_mode(samplerInfo.filter),
        .addressModeU = convert_address_mode(samplerInfo.address_u),
        .addressModeV = convert_address_mode(samplerInfo.address_v),
        .addressModeW = convert_address_mode(samplerInfo.address_w),
        .mipLodBias = samplerInfo.lod_bias,
        .anisotropyEnable = samplerInfo.filter == reshadefx::filter_mode::anisotropic ? VK_TRUE : VK_FALSE,
        .maxAnisotropy = 16.0f,  // TODO: Query device for max anisotropy
        .minLod = samplerInfo.min_lod,
        .maxLod = samplerInfo.max_lod,
    };

	VK_CHECK(m_device.dispatch.CreateSampler(m_device.handle, &createInfo, nullptr, &m_sampler));
}

vkShade::VulkanSampler::~VulkanSampler()
{
    Logger::trace("Destroying sampler");
    m_device.dispatch.DestroySampler(m_device.handle, m_sampler, nullptr);
}

VkSamplerAddressMode vkShade::VulkanSampler::convert_address_mode(reshadefx::texture_address_mode addressMode)
{
    switch (addressMode)
    {
        case reshadefx::texture_address_mode::wrap:     return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        case reshadefx::texture_address_mode::mirror:   return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        case reshadefx::texture_address_mode::clamp:    return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        case reshadefx::texture_address_mode::border:   return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    }

    std::unreachable();
}

VkFilter vkShade::VulkanSampler::convert_mag_filter(reshadefx::filter_mode filter)
{
    switch (filter)
    {
        case reshadefx::filter_mode::min_mag_mip_point:
        case reshadefx::filter_mode::min_mag_point_mip_linear:
        case reshadefx::filter_mode::min_linear_mag_mip_point:
        case reshadefx::filter_mode::min_linear_mag_point_mip_linear:
            return VK_FILTER_NEAREST;

        case reshadefx::filter_mode::min_point_mag_linear_mip_point:
        case reshadefx::filter_mode::min_point_mag_mip_linear:
        case reshadefx::filter_mode::min_mag_linear_mip_point:
        case reshadefx::filter_mode::min_mag_mip_linear:
        case reshadefx::filter_mode::anisotropic:
            return VK_FILTER_LINEAR;
    }

    std::unreachable();
}

VkFilter vkShade::VulkanSampler::convert_min_filter(reshadefx::filter_mode filter)
{
    switch (filter)
    {
        case reshadefx::filter_mode::min_mag_mip_point:
        case reshadefx::filter_mode::min_mag_point_mip_linear:
        case reshadefx::filter_mode::min_point_mag_linear_mip_point:
        case reshadefx::filter_mode::min_point_mag_mip_linear:
            return VK_FILTER_NEAREST;

        case reshadefx::filter_mode::min_linear_mag_mip_point:
        case reshadefx::filter_mode::min_linear_mag_point_mip_linear:
        case reshadefx::filter_mode::min_mag_linear_mip_point:
        case reshadefx::filter_mode::min_mag_mip_linear:
        case reshadefx::filter_mode::anisotropic:
            return VK_FILTER_LINEAR;
    }

    std::unreachable();
}

VkSamplerMipmapMode vkShade::VulkanSampler::convert_mipmap_mode(reshadefx::filter_mode filter)
{
    switch (filter)
    {
        case reshadefx::filter_mode::min_mag_mip_point:
        case reshadefx::filter_mode::min_point_mag_linear_mip_point:
        case reshadefx::filter_mode::min_linear_mag_mip_point:
        case reshadefx::filter_mode::min_mag_linear_mip_point:
            return VK_SAMPLER_MIPMAP_MODE_NEAREST;

        case reshadefx::filter_mode::min_mag_point_mip_linear:
        case reshadefx::filter_mode::min_point_mag_mip_linear:
        case reshadefx::filter_mode::min_linear_mag_point_mip_linear:
        case reshadefx::filter_mode::min_mag_mip_linear:
        case reshadefx::filter_mode::anisotropic:
            return VK_SAMPLER_MIPMAP_MODE_LINEAR;
    }

    std::unreachable();
}
