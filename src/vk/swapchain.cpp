#include "swapchain.hpp"

#include <magic_enum/magic_enum.hpp>
#include <spdlog/spdlog.h>

#include "hooks/hooks.hpp"

vkShade::VulkanSwapchain::VulkanSwapchain(VkDevice device, VkSwapchainKHR swapchain, VkSwapchainCreateInfoKHR swapchainInfo)
    : VulkanObject(device)
{
    // Store the swapchain info
    m_swapchain = swapchain;
    m_format = swapchainInfo.imageFormat;
    m_extent = swapchainInfo.imageExtent;

    auto& thisDevice = g_vulkanDevices[dispatch_key_from_handle(device)];

    // Get swapchain images
    uint32_t imageCount = 0;
    thisDevice.dispatch.GetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
    m_images.resize(imageCount);
    thisDevice.dispatch.GetSwapchainImagesKHR(device, swapchain, &imageCount, m_images.data());

    // Create image views
    m_imageViews.resize(imageCount);
    for (uint32_t i = 0; i < imageCount; i++)
    {
        VkImageViewCreateInfo viewInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = m_images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = swapchainInfo.imageFormat,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        };
        thisDevice.dispatch.CreateImageView(device, &viewInfo, nullptr, &m_imageViews[i]);
    }
}

VkImage vkShade::VulkanSwapchain::image(size_t index) const
{
    assert(index < m_images.size());
    return m_images[index];
}

VkImageView vkShade::VulkanSwapchain::image_view(uint32_t index) const
{
    assert(index < m_imageViews.size());
    return m_imageViews[index];
}
