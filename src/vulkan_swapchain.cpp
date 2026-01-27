#include "vulkan_swapchain.hpp"

#include <magic_enum/magic_enum.hpp>
#include <spdlog/spdlog.h>

#include "vulkan_hooks.hpp"

vkShade::VulkanSwapchain::VulkanSwapchain(VkDevice device, VkSwapchainKHR swapchain, VkSwapchainCreateInfoKHR swapchainInfo)
{
    // Store the swapchain info
    m_device = device;
    m_swapchain = swapchain;
    m_format = swapchainInfo.imageFormat;
    m_extent = swapchainInfo.imageExtent;

    auto& thisDevice = g_vulkanDevices[dispatch_key_from_handle(device)];

    // Get swapchain images
    uint32_t imageCount = 0;
    thisDevice.dispatch.GetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
    std::vector<VkImage> images(imageCount);
    thisDevice.dispatch.GetSwapchainImagesKHR(device, swapchain, &imageCount, images.data());
}
