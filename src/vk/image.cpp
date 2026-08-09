#include "image.hpp"

#include <effect_module.hpp>
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_image_resize2.h>
#include <vulkan/vulkan_core.h>

#include "core/service_locator.hpp"
#include "core/logger.hpp"
#include "config/config_manager.hpp"
#include "buffer.hpp"
#include "initializers.hpp"
#include "macros.hpp"

vkShade::VulkanImage::VulkanImage(VulkanDevice& device, const reshadefx::texture& info)
    : VulkanObject(device)
{
    Logger::trace("Creating image from ReShade FX info");

    m_extent.width  = info.width;
    m_extent.height = info.height;
    m_extent.depth  = info.depth;

    m_format = convert_format(info.format);

    // Format undefined is UB, so fallback to RGBA8 UNORM.
    if (m_format == VK_FORMAT_UNDEFINED)
        m_format = VK_FORMAT_R8G8B8A8_UNORM;

    m_usageFlags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    if (info.render_target)
        m_usageFlags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if (info.storage_access)
        m_usageFlags |= VK_IMAGE_USAGE_STORAGE_BIT;

    // If image has a source (file) annotation, load it.
    auto it = std::find_if(info.annotations.begin(), info.annotations.end(), [](const auto& a)
    {
        return a.name == "source";
    });

    // If no explicit size is specified we need to extract it from the image file
    if (it != info.annotations.end() && m_extent.width == 1 && m_extent.height == 1)
    {
        // Find the file and probe dimensions before creating the image
        auto& config = vkShade::Locator<vkShade::ConfigManager>::get().app();
        auto searchPaths = config.get<std::vector<std::string>>("ReShade", "TextureSearchPaths");
        std::string fileName = it->value.string_data;

        bool found = false;
        for (auto& path : searchPaths.value_or(std::vector<std::string>{}))
        {
            for (const auto& entry : std::filesystem::directory_iterator(path))
            {
                // Do a case insensitive comparison since ReShade is a Windows app
                auto a = entry.path().filename().string();
                auto b = fileName;
                std::transform(a.begin(), a.end(), a.begin(), ::tolower);
                std::transform(b.begin(), b.end(), b.begin(), ::tolower);
                if (a == b)
                {
                    int w, h, c;
                    if (stbi_info(entry.path().c_str(), &w, &h, &c))
                    {
                        m_extent.width  = w;
                        m_extent.height = h;
                    }

                    found = true;
                    break;
                }
            }
        }

        if (!found)
            throw std::runtime_error("Unable to find texture: " + fileName);
    }

    this->create_image();
    this->create_image_view();

    if (it != info.annotations.end())
    {
        auto& config = vkShade::Locator<vkShade::ConfigManager>::get().app();
        auto searchPaths = config.get<std::vector<std::string>>("ReShade", "TextureSearchPaths");
        std::string fileName = it->value.string_data;

        bool found = false;
        for (auto& path : searchPaths.value_or(std::vector<std::string>{}))
        {
            for (const auto& entry : std::filesystem::directory_iterator(path))
            {
                // Do a case insensitive comparison since ReShade is a Windows app
                auto a = entry.path().filename().string();
                auto b = fileName;
                std::transform(a.begin(), a.end(), a.begin(), ::tolower);
                std::transform(b.begin(), b.end(), b.begin(), ::tolower);
                if (a == b)
                {
                    this->load_from_file(entry.path());
                    found = true;
                    break;
                }
            }
        }

        if (!found)
            throw std::runtime_error("Unable to find texture: " + fileName);
    }

    this->register_tracking(TrackedImageOrigin::VkShade);
}

void vkShade::VulkanImage::load_from_file(const std::string& filePath)
{
    Logger::trace("Creating image from file: {}", filePath);

    int32_t channels;
    stbir_pixel_layout stbirLayout;
    switch (m_format)
    {
        case VK_FORMAT_R8_UNORM:
            channels = 1;
            stbirLayout = STBIR_1CHANNEL;
            break;
        case VK_FORMAT_R8G8_UNORM:
            channels = 2;
            stbirLayout = STBIR_2CHANNEL;
            break;
        case VK_FORMAT_R8G8B8A8_UNORM:
            channels = 4;
            stbirLayout = STBIR_RGBA;
            break;
        default:
            throw std::runtime_error("Unsupported format for file-sourced texture: " +
                std::string(magic_enum::enum_name(m_format)));
    }

    int32_t srcWidth, srcHeight, srcChannels;
    stbi_uc* pixels = stbi_load(filePath.c_str(), &srcWidth, &srcHeight, &srcChannels, channels);

    if (!pixels)
    {
        Logger::error("Failed to load image: {}", filePath);
        throw std::runtime_error("Failed to load image: " + filePath);
    }

    // Resize the image if it doesn't match the requested size
    const void* uploadData = pixels;
    std::vector<uint8_t> resized;

    if (srcWidth != (int32_t)m_extent.width || srcHeight != (int32_t)m_extent.height)
    {
        resized.resize(m_extent.width * m_extent.height * channels);
        stbir_resize_uint8_linear(pixels, srcWidth, srcHeight, 0,
                                  resized.data(), m_extent.width, m_extent.height, 0,
                                  stbirLayout);
        uploadData = resized.data();
    }

    size_t imageSize = m_extent.width * m_extent.height * channels;
    this->upload(uploadData, imageSize);

    stbi_image_free(pixels);
}

vkShade::VulkanImage::VulkanImage(VulkanDevice& device, VkExtent2D size, VkFormat format, VkImageUsageFlags usageFlags)
    : VulkanObject(device)
{
    Logger::trace("Creating image (Size: {}x{}, Format: {})",
        size.width, size.height, magic_enum::enum_name(format));

    // Set internal properties
    m_extent.width  = size.width;
    m_extent.height = size.height;
    m_extent.depth  = 1;
    m_format = format;
    m_usageFlags = usageFlags;

    this->create_image();
    this->create_image_view();
    this->register_tracking(TrackedImageOrigin::VkShade);
}

vkShade::VulkanImage::VulkanImage(VulkanDevice& device, VkImage image, VkExtent2D size,
                                  VkFormat format, VkImageUsageFlags usageFlags,
                                  uint32_t arrayLayers)
    : VulkanObject(device)
{
    Logger::trace("Wrapping existing image (Size: {}x{}, Format: {})",
        size.width, size.height, magic_enum::enum_name(format));

    // Set internal properties
    m_image = image;
    m_extent.width  = size.width;
    m_extent.height = size.height;
    m_extent.depth  = 1;
    m_format = format;
    m_usageFlags = usageFlags;
    m_owning = false;  // IMPORTANT! We don't own the image here.

    // Only create the view (no image since we don't own it)
    this->create_image_view();
    this->register_tracking(TrackedImageOrigin::Swapchain, arrayLayers);
}

vkShade::VulkanImage::~VulkanImage()
{
    Logger::trace("Destroying VulkanImage");

    try
    {
        m_device.imageTracker->unregister_view(m_imageView);
        m_device.imageTracker->unregister_image(m_image);
    }
    catch (const std::exception& exception)
    {
        Logger::warn("Buffer tracking failed: {}", exception.what());
    }

    if (m_imageView != VK_NULL_HANDLE)
        m_device.dispatch.DestroyImageView(m_device.handle, m_imageView, nullptr);

    if (m_owning && m_image != VK_NULL_HANDLE && m_allocation != VK_NULL_HANDLE)
        vmaDestroyImage(m_device.allocator, m_image, m_allocation);
}

void vkShade::VulkanImage::create_image()
{
    VkImageCreateInfo imgInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = nullptr,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = m_format,
        .extent = VkExtent3D {m_extent.width, m_extent.height, 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,   // 1 sample per pixel = no MSAA
        .tiling = VK_IMAGE_TILING_OPTIMAL,  // Let the GPU move the data as it sees fit
        .usage = m_usageFlags,
    };

	// Always allocate images on dedicated GPU memory.
	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	allocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	// Allocate and create the image.
	VK_CHECK(vmaCreateImage(m_device.allocator, &imgInfo, &allocInfo, &m_image, &m_allocation, nullptr));
}

void vkShade::VulkanImage::create_image_view()
{
    // If the format is a depth format, we will need to have it use the correct aspect flag.
	VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
	if (m_format == VK_FORMAT_D32_SFLOAT)
		aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
	if (m_format == VK_FORMAT_S8_UINT)
		aspectFlags = VK_IMAGE_ASPECT_STENCIL_BIT;

	// Build an image view for the image.
    VkImageViewCreateInfo viewInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext = nullptr,
        .image = m_image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = m_format,
        .subresourceRange = {
            .aspectMask = aspectFlags,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
    };

	VK_CHECK(m_device.dispatch.CreateImageView(m_device.handle, &viewInfo, nullptr, &m_imageView));
}

void vkShade::VulkanImage::register_tracking(TrackedImageOrigin origin,
                                             uint32_t arrayLayers)
{
    const VkImageCreateInfo imageInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = m_format,
        .extent = {m_extent.width, m_extent.height, 1},
        .mipLevels = 1,
        .arrayLayers = arrayLayers,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = m_usageFlags,
    };
    const VkImageSubresourceRange viewRange = {
        .aspectMask = m_format == VK_FORMAT_D32_SFLOAT
                    ? VK_IMAGE_ASPECT_DEPTH_BIT
                    : m_format == VK_FORMAT_S8_UINT
                    ? VK_IMAGE_ASPECT_STENCIL_BIT
                    : VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
    };

    try
    {
        m_device.imageTracker->register_image(m_image, imageInfo, origin);
        m_device.imageTracker->register_view(m_imageView, m_image, viewRange);
    }
    catch (const std::exception& exception)
    {
        try
        {
            m_device.imageTracker->unregister_view(m_imageView);
            m_device.imageTracker->unregister_image(m_image);
        }
        catch (...)
        {
        }
        Logger::warn("Buffer tracking failed: {}", exception.what());
    }
    catch (...)
    {
        try
        {
            m_device.imageTracker->unregister_view(m_imageView);
            m_device.imageTracker->unregister_image(m_image);
        }
        catch (...)
        {
        }
        Logger::warn("Buffer tracking failed with an unknown error");
    }
}

void vkShade::VulkanImage::blit_from(VkCommandBuffer cmd, VkImage source)
{
    blit(cmd, source, m_image);
}

void vkShade::VulkanImage::blit_to(VkCommandBuffer cmd, VkImage destination)
{
    blit(cmd, m_image, destination);
}

void vkShade::VulkanImage::blit(VkCommandBuffer cmd, VkImage source, VkImage destination)
{
	VkImageBlit2 blitRegion{ .sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2, .pNext = nullptr };

    blitRegion.srcOffsets[0] = { 0, 0, 0 };
    blitRegion.srcOffsets[1] = { (int32_t)m_extent.width, (int32_t)m_extent.height, 1 };

    blitRegion.dstOffsets[0] = { 0, 0, 0 };
    blitRegion.dstOffsets[1] = { (int32_t)m_extent.width, (int32_t)m_extent.height, 1 };

	blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	blitRegion.srcSubresource.baseArrayLayer = 0;
	blitRegion.srcSubresource.layerCount = 1;
	blitRegion.srcSubresource.mipLevel = 0;

	blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	blitRegion.dstSubresource.baseArrayLayer = 0;
	blitRegion.dstSubresource.layerCount = 1;
	blitRegion.dstSubresource.mipLevel = 0;

	VkBlitImageInfo2 blitInfo{ .sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2, .pNext = nullptr };
	blitInfo.srcImage = m_image;
	blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	blitInfo.dstImage = destination;
	blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	blitInfo.filter = VK_FILTER_LINEAR;
	blitInfo.regionCount = 1;
	blitInfo.pRegions = &blitRegion;

	m_device.dispatch.CmdBlitImage2(cmd, &blitInfo);
}

VkFormat vkShade::VulkanImage::convert_format(reshadefx::texture_format format)
{
    switch (format)
    {
        case reshadefx::texture_format::unknown:    return VK_FORMAT_UNDEFINED;

        case reshadefx::texture_format::r8:         return VK_FORMAT_R8_UNORM;
        case reshadefx::texture_format::r16f:       return VK_FORMAT_R16_SFLOAT;
        case reshadefx::texture_format::r16:        return VK_FORMAT_R16_UNORM;
        case reshadefx::texture_format::r32f:       return VK_FORMAT_R32_SFLOAT;
        case reshadefx::texture_format::r32u:       return VK_FORMAT_R32_UINT;
        case reshadefx::texture_format::r32i:       return VK_FORMAT_R32_SINT;

        case reshadefx::texture_format::rg8:        return VK_FORMAT_R8G8_UNORM;
        case reshadefx::texture_format::rg16f:      return VK_FORMAT_R16G16_SFLOAT;
        case reshadefx::texture_format::rg16:       return VK_FORMAT_R16G16_UNORM;
        case reshadefx::texture_format::rg32f:      return VK_FORMAT_R32G32_SFLOAT;

        case reshadefx::texture_format::rgba8:      return VK_FORMAT_R8G8B8A8_UNORM;
        case reshadefx::texture_format::rgba16f:    return VK_FORMAT_R16G16B16A16_SFLOAT;
        case reshadefx::texture_format::rgba16:     return VK_FORMAT_R16G16B16A16_UNORM;
        case reshadefx::texture_format::rgba32f:    return VK_FORMAT_R32G32B32A32_SFLOAT;
        case reshadefx::texture_format::rgba32u:    return VK_FORMAT_R32G32B32A32_UINT;
        case reshadefx::texture_format::rgba32i:    return VK_FORMAT_R32G32B32A32_SINT;

        case reshadefx::texture_format::rgb10a2:    return VK_FORMAT_A2B10G10R10_UNORM_PACK32;
        case reshadefx::texture_format::rg11b10f:   return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
    }

    std::unreachable();
}

void vkShade::VulkanImage::transition_layout(VkCommandBuffer cmd, VkImageLayout newLayout)
{
    VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    if (m_format == VK_FORMAT_D32_SFLOAT)
        aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    if (m_format == VK_FORMAT_S8_UINT)
        aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT;

    VkImageMemoryBarrier2 imageBarrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
        .srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
        .dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT,
        .oldLayout = m_currentLayout,  // Track current layout
        .newLayout = newLayout,
        .image = m_image,
        .subresourceRange = {
            .aspectMask = aspectMask,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
    };

    VkDependencyInfo depInfo = {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &imageBarrier,
    };

    m_device.dispatch.CmdPipelineBarrier2(cmd, &depInfo);

    m_currentLayout = newLayout;  // Update tracked layout
}

void vkShade::VulkanImage::upload(const void* data, size_t size)
{
    // Create staging buffer
    VulkanBuffer staging(m_device, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
    staging.write(data, size);

    // Record and submit a one-time command buffer
    VkCommandBufferAllocateInfo cmdBufAllocInfo = vkinit::command_buffer_allocate_info(m_device.commandPool);

    VkCommandBuffer cmd;

    // NOTE: Command buffers are dispatchable types. Their creation must go through our
    //  own hook, not the dispatch table, to ensure the dispatch pointer fixup runs.
    vkShade_AllocateCommandBuffers(m_device.handle, &cmdBufAllocInfo, &cmd);

    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    m_device.dispatch.BeginCommandBuffer(cmd, &beginInfo);

    // Transition UNDEFINED -> TRANSFER_DST_OPTIMAL
    this->transition_layout(cmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    VkBufferImageCopy copyRegion = {
        .imageSubresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
        .imageExtent = {
            .width = m_extent.width,
            .height = m_extent.height,
            .depth = 1
        }
    };

    m_device.dispatch.CmdCopyBufferToImage(cmd, staging.buffer(), m_image,
                                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

    // Transition TRANSFER_DST_OPTIMAL -> SHADER_READ_ONLY_OPTIMAL
    this->transition_layout(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    m_device.dispatch.EndCommandBuffer(cmd);

    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &cmd,
    };

    VK_CHECK(m_device.dispatch.QueueSubmit(m_device.queue, 1, &submitInfo, VK_NULL_HANDLE));
    VK_CHECK(m_device.dispatch.QueueWaitIdle(m_device.queue));

    m_device.dispatch.FreeCommandBuffers(m_device.handle, m_device.commandPool, 1, &cmd);
}
