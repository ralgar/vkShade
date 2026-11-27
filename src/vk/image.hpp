#pragma once

#include <functional>
#include <optional>
#include <string>

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

        // Regenerate all levels below mip 0
        void generate_mipmaps(VkCommandBuffer cmd);

        // Transition the image from its current layout into a new layout
        void transition_layout(VkCommandBuffer cmd, VkImageLayout newLayout);

        // Getters for handles
        const VkImage& image() const { return m_image; }
        const VkImageView& all_mips_view() const { return m_allMipsView; }
        const VkImageView& base_mip_view() const { return m_baseMipView; }
        const VkFormat format() const { return m_format; }
        const VkExtent3D extent() const { return m_extent; }

    private:
        // Delegation target, so the destructor runs if a delegating constructor throws.
        explicit VulkanImage(VulkanDevice& device) : VulkanObject(device) {}

        bool               m_owning = true;
        VkExtent3D         m_extent {0, 0, 0};
        VkImage            m_image = VK_NULL_HANDLE;
        VkImageView        m_allMipsView = VK_NULL_HANDLE;
        VkImageView        m_baseMipView = VK_NULL_HANDLE;
        VkImageUsageFlags  m_usageFlags = 0;
        VkImageAspectFlags m_aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
        VmaAllocation      m_allocation = VK_NULL_HANDLE;
        VkFormat           m_format = VK_FORMAT_UNDEFINED;
        VkImageLayout      m_currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        uint32_t           m_mipLevels = 1;

        // Filter for mip generation, empty if this image has no mips or can't blit them.
        std::optional<VkFilter> m_mipFilter;

        // The actual blit implementation
        void blit(VkCommandBuffer cmd, VkImage src, VkImage dest);

        void clear_to_zero();
        void create_image();
        void create_image_view();

        void immediate_submit(const std::function<void(VkCommandBuffer cmd)>& function);

        void load_from_file(const std::string& filePath);

        void upload(const void* data, size_t size);

        static VkFormat convert_format(reshadefx::texture_format format);
    };
} // namespace vkShade
