#include "vulkan_hooks.hpp"
#include "vulkan_swapchain.hpp"

#include <spdlog/spdlog.h>

std::unordered_map<VkSwapchainKHR, vkShade::VulkanSwapchain> g_swapchains;
std::mutex g_swapchainMutex;

VK_LAYER_EXPORT VkResult VKAPI_CALL vkShade_CreateSwapchainKHR(VkDevice                        device,
                                                               const VkSwapchainCreateInfoKHR* pCreateInfo,
                                                               const VkAllocationCallbacks*    pAllocator,
                                                               VkSwapchainKHR*                 pSwapchain)
{
    spdlog::trace("vkCreateSwapchainKHR called");

    auto& deviceData = g_vulkanDevices[dispatch_key_from_handle(device)];

    // Call through to create the actual swapchain
    VkResult result = deviceData.dispatchTable.CreateSwapchainKHR(device, pCreateInfo, pAllocator, pSwapchain);
    if (result != VK_SUCCESS)
        return result;

    // Create and store swapchain object
    vkShade::VulkanSwapchain swapchain(deviceData.device, *pSwapchain, *pCreateInfo);
    {
        std::lock_guard<std::mutex> lock(g_swapchainMutex);
        g_swapchains.insert({*pSwapchain, std::move(swapchain)});
    }

    spdlog::debug("Swapchain created");
    return VK_SUCCESS;
}

VK_LAYER_EXPORT void VKAPI_CALL vkShade_DestroySwapchainKHR(VkDevice                     device,
                                                            VkSwapchainKHR               swapchain,
                                                            const VkAllocationCallbacks* pAllocator)
{
    spdlog::trace("vkDestroySwapchainKHR called");

    // Clean up our bookkeeping data
    {
        std::lock_guard<std::mutex> lock(g_swapchainMutex);
        g_swapchains.erase(swapchain);
    }

    // Call through
    auto& deviceData = g_vulkanDevices[dispatch_key_from_handle(device)];
    deviceData.dispatchTable.DestroySwapchainKHR(device, swapchain, pAllocator);

    spdlog::debug("Swapchain destroyed");
}

VK_LAYER_EXPORT VkResult VKAPI_CALL vkShade_QueuePresentKHR(VkQueue queue, const VkPresentInfoKHR *pPresentInfo)
{
    spdlog::debug("Queueing present");

    return g_vulkanDevices[dispatch_key_from_handle(queue)].dispatchTable.QueuePresentKHR(queue, pPresentInfo);
}
