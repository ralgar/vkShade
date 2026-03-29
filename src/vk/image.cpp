#include "image.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <vulkan/vulkan_core.h>

#include "buffer.hpp"
#include "initializers.hpp"
#include "macros.hpp"

vkShade::VulkanImage::VulkanImage(VulkanDevice& device, const std::string& filePath, VkImageUsageFlags usageFlags)
    : VulkanObject(device)
{
    spdlog::trace("Creating image from file: {}", filePath);

    int32_t texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(filePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    size_t imageSize = texWidth * texHeight * 4;

    m_extent.width  = texWidth;
    m_extent.height = texHeight;
    m_format = VK_FORMAT_R8G8B8A8_UNORM;
    m_usageFlags = usageFlags;

    if (!pixels)
    {
        spdlog::error("Failed to load image: {}", filePath);
        throw std::runtime_error("Failed to load image");
    }

    this->create_image();
    this->create_image_view();

    this->upload(pixels, imageSize);

    stbi_image_free(pixels);
}

vkShade::VulkanImage::VulkanImage(VulkanDevice& device, VkExtent2D size, VkFormat format, VkImageUsageFlags usageFlags)
    : VulkanObject(device)
{
    spdlog::trace("Creating image (Size: {}x{}, Format: {})",
        size.width, size.height, magic_enum::enum_name(format));

    // Set internal properties
    m_extent = size;
    m_format = format;
    m_usageFlags = usageFlags;

    this->create_image();
    this->create_image_view();
}

vkShade::VulkanImage::VulkanImage(VulkanDevice& device, VkImage image, VkExtent2D size, VkFormat format)
    : VulkanObject(device)
{
    spdlog::trace("Wrapping existing image (Size: {}x{}, Format: {})",
        size.width, size.height, magic_enum::enum_name(format));

    // Set internal properties
    m_image = image;
    m_extent = size;
    m_format = format;
    m_owning = false;  // IMPORTANT! We don't own the image here.

    // Only create the view (no image since we don't own it)
    this->create_image_view();
}

vkShade::VulkanImage::~VulkanImage()
{
    spdlog::trace("Destroying VulkanImage");

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
	if (m_format == VK_FORMAT_D32_SFLOAT) {
		aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
	}

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

	blitRegion.srcOffsets[0].x = m_extent.width;
	blitRegion.srcOffsets[0].y = m_extent.height;
	blitRegion.srcOffsets[0].z = 1;

	blitRegion.dstOffsets[0].x = m_extent.width;
	blitRegion.dstOffsets[0].y = m_extent.height;
	blitRegion.dstOffsets[0].z = 1;

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

void vkShade::VulkanImage::transition_layout(VkCommandBuffer cmd, VkImageLayout newLayout)
{
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
            .aspectMask = (newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL)
                ? VK_IMAGE_ASPECT_DEPTH_BIT
                : VK_IMAGE_ASPECT_COLOR_BIT,
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

void vkShade::VulkanImage::upload(void* data, size_t size)
{
    // Create staging buffer
    VulkanBuffer staging(m_device, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
    staging.write(data, size);

    // Record and submit a one-time command buffer
    VkCommandBufferAllocateInfo cmdBufAllocInfo = vkinit::command_buffer_allocate_info(m_device.commandPool);

    VkCommandBuffer cmd;
    m_device.dispatch.AllocateCommandBuffers(m_device.handle, &cmdBufAllocInfo, &cmd);

    VkCommandBufferBeginInfo beginInfo;
    m_device.dispatch.BeginCommandBuffer(cmd, &beginInfo);

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

    m_device.dispatch.EndCommandBuffer(cmd);

    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &cmd,
    };

    m_device.dispatch.QueueSubmit(m_device.queue, 1, &submitInfo, VK_NULL_HANDLE);
    m_device.dispatch.QueueWaitIdle(m_device.queue);

    m_device.dispatch.FreeCommandBuffers(m_device.handle, m_device.commandPool, 1, &cmd);
}
