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
}

VkImage vkShade::VulkanSwapchain::image(size_t index) const
{
    assert(index < m_images.size());
    return m_images[index];
}
