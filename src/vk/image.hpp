#pragma once

#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

#include "object.hpp"

namespace vkShade
{
    class VulkanImage : public VulkanObject
    {
    public:
        // Owning constructor: creates image + view
        VulkanImage(VulkanDevice& device, VkExtent2D size, VkFormat format, VkImageUsageFlags usageFlags);

        // Non-owning constructor: wraps existing image + creates its own view
        VulkanImage(VulkanDevice& device, VkImage image, VkExtent2D size, VkFormat format);

        ~VulkanImage() override;

        // Blit FROM another image INTO this image (assumes same size, mip 0)
        void blit_from(VkCommandBuffer cmd, VkImage source);

        // Blit FROM this image INTO another image (assumes same size, mip 0)
        void blit_to(VkCommandBuffer cmd, VkImage destination);

        // Transition the image from its current layout into a new layout
        void transition_layout(VkCommandBuffer cmd, VkImageLayout newLayout);

        // Getters for handles
        const VkImage& image() const { return m_image; }
        const VkImageView& image_view() const { return m_imageView; }

    private:
        bool              m_owning = true;
        VkExtent2D        m_extent {0, 0};
        VkImage           m_image = VK_NULL_HANDLE;
        VkImageView       m_imageView = VK_NULL_HANDLE;
        VkImageUsageFlags m_usageFlags = 0;
        VmaAllocation     m_allocation = VK_NULL_HANDLE;
        VkFormat          m_format = VK_FORMAT_UNDEFINED;
        VkImageLayout     m_currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        // The actual blit implementation
        void blit(VkCommandBuffer cmd, VkImage src, VkImage dest);

        void create_image();
        void create_image_view();
    };
} // namespace vkShade
