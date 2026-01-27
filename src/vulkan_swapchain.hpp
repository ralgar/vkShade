#pragma once

#include <vector>
#include <vulkan/vulkan_core.h>

#include "vulkan_object.hpp"

namespace vkShade
{
    class VulkanSwapchain : public VulkanObject
    {
    public:
        VulkanSwapchain(VkDevice device, VkSwapchainKHR swapchain, VkSwapchainCreateInfoKHR swapchainInfo);

        uint32_t image_count() const { return m_images.size(); }

    private:
        VkSwapchainKHR m_swapchain;
        VkFormat m_format;
        VkExtent2D m_extent;
        std::vector<VkImage> m_images;
    };
}  // namespace vkShade
