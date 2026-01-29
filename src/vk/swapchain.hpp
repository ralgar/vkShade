#pragma once

#include <vector>
#include <vulkan/vulkan_core.h>

#include "object.hpp"

namespace vkShade
{
    class VulkanSwapchain : public VulkanObject
    {
    public:
        VulkanSwapchain(VkDevice device, VkSwapchainKHR swapchain, VkSwapchainCreateInfoKHR swapchainInfo);

        VkExtent2D extent() const { return m_extent; }
        VkFormat format() const { return m_format; }
        VkImage image(size_t index) const;
        VkImageView image_view(uint32_t index) const;
        uint32_t image_count() const { return m_images.size(); }

    private:
        VkSwapchainKHR m_swapchain;
        VkFormat m_format;
        VkExtent2D m_extent;
        std::vector<VkImage> m_images;
        std::vector<VkImageView> m_imageViews;
    };
}  // namespace vkShade
