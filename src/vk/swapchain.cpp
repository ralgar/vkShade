#include "swapchain.hpp"

#include <magic_enum/magic_enum.hpp>
#include <spdlog/spdlog.h>

#include "hooks/hooks.hpp"
#include "image.hpp"

vkShade::VulkanSwapchain::VulkanSwapchain(VulkanDevice& device, VkSwapchainKHR swapchain, VkSwapchainCreateInfoKHR swapchainInfo)
    : VulkanObject(device)
{
    // Store the swapchain info
    m_swapchain = swapchain;
    m_format = swapchainInfo.imageFormat;
    m_extent = swapchainInfo.imageExtent;

    // Get swapchain images
    uint32_t imageCount = 0;
    m_device.dispatch.GetSwapchainImagesKHR(m_device.handle, swapchain, &imageCount, nullptr);
    std::vector<VkImage> images(imageCount);
    m_device.dispatch.GetSwapchainImagesKHR(m_device.handle, swapchain, &imageCount, images.data());

    // Create image views
    for (auto& image : images)
    {
        m_images.push_back(std::make_unique<VulkanImage>(device, image, m_extent, m_format));
    }

	VkImageUsageFlags drawImageUsages {};
	drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_SAMPLED_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    m_pingPongA = std::shared_ptr<VulkanImage>(new VulkanImage(m_device, m_extent, m_format, drawImageUsages));
    m_pingPongB = std::shared_ptr<VulkanImage>(new VulkanImage(m_device, m_extent, m_format, drawImageUsages));
}

vkShade::VulkanImage& vkShade::VulkanSwapchain::image(size_t index) const
{
    assert(index < m_images.size());
    return *m_images[index];
}
