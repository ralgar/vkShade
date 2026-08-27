#pragma once

#include <string>
#include <vector>

#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

#include "object.hpp"

namespace reshadefx
{
    struct texture;
    enum class texture_format : uint8_t;
}

namespace vkShade
{
    class VulkanImage : public VulkanObject
    {
    public:
        // Owning constructor: creates or loads an image according to info struct
        VulkanImage(VulkanDevice& device, const reshadefx::texture& info);

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

        // Transition only the mip level used as a color attachment.
        void transition_render_target_layout(VkCommandBuffer cmd, VkImageLayout newLayout);

        // Regenerate all levels below mip 0.
        void generate_mipmaps(VkCommandBuffer cmd);

        // Initialize effect-owned images before their first use.
        void initialize(VkCommandBuffer cmd);

        // Getters for handles
        const VkImage& image() const { return m_image; }
        const VkImageView& image_view() const;
        const VkImageView& sampled_view(bool srgb) const;
        const VkImageView& render_target_view() const;
        const VkImageView& render_target_view(bool srgb) const;
        const VkFormat format() const { return m_format; }
        const VkExtent3D extent() const { return m_extent; }
        uint32_t mip_levels() const { return m_mipLevels; }

        static VkFormat view_format(VkFormat format, bool srgb);

    private:
        bool              m_owning = true;
        VkExtent3D        m_extent {0, 0, 0};
        VkImage           m_image = VK_NULL_HANDLE;
        VkImageView       m_linearImageView = VK_NULL_HANDLE;
        VkImageView       m_srgbImageView = VK_NULL_HANDLE;
        VkImageView       m_linearRenderTargetView = VK_NULL_HANDLE;
        VkImageView       m_srgbRenderTargetView = VK_NULL_HANDLE;
        VkImageUsageFlags m_usageFlags = 0;
        VmaAllocation     m_allocation = VK_NULL_HANDLE;
        VkFormat          m_format = VK_FORMAT_UNDEFINED;
        uint32_t          m_mipLevels = 1;
        std::vector<VkImageLayout> m_mipLayouts;
        bool              m_needsInitialization = false;

        // The actual blit implementation
        void blit(VkCommandBuffer cmd, VkImage src, VkImage dest);

        void create_image();
        void create_image_view();
        void destroy();

        void load_from_file(const std::string& filePath);

        void upload(const void* data, size_t size);

        void transition_layout(VkCommandBuffer cmd, VkImageLayout newLayout,
                               uint32_t baseMipLevel, uint32_t levelCount);

        static VkFormat convert_format(reshadefx::texture_format format);
    };
} // namespace vkShade
