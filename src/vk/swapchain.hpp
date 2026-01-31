#pragma once

#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "object.hpp"

namespace vkShade
{
    class VulkanImage;

    class VulkanSwapchain : public VulkanObject
    {
    public:
        VulkanSwapchain(VulkanDevice& device, VkSwapchainKHR swapchain, VkSwapchainCreateInfoKHR swapchainInfo);

        VkExtent2D extent() const { return m_extent; }
        VkFormat format() const { return m_format; }
        VulkanImage& image(size_t index) const;
        uint32_t image_count() const { return m_images.size(); }

        // Ping-pong image accessors
        VulkanImage& ping_pong_a() { return *m_pingPongA; }
        VulkanImage& ping_pong_b() { return *m_pingPongB; }

    private:
        VkSwapchainKHR m_swapchain;
        VkFormat m_format;
        VkExtent2D m_extent;
        std::vector<std::unique_ptr<VulkanImage>> m_images;

        std::shared_ptr<VulkanImage> m_pingPongA;
        std::shared_ptr<VulkanImage> m_pingPongB;
    };
}  // namespace vkShade
