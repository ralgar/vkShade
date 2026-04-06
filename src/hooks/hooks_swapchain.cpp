#include "gui/gui_manager.hpp"
#include "hooks.hpp"
#include "vk/swapchain.hpp"

#include <imgui.h>
#include <imgui_impl_vulkan.h>
#include <vulkan/vulkan_core.h>
#include <spdlog/spdlog.h>

#include "core/service_locator.hpp"
#include "input/input_manager.hpp"

std::unordered_map<VkSwapchainKHR, std::unique_ptr<vkShade::VulkanSwapchain>> g_swapchains;
std::mutex g_swapchainMutex;

VK_LAYER_EXPORT VkResult VKAPI_CALL vkShade_CreateSwapchainKHR(VkDevice                        device,
                                                               const VkSwapchainCreateInfoKHR* pCreateInfo,
                                                               const VkAllocationCallbacks*    pAllocator,
                                                               VkSwapchainKHR*                 pSwapchain)
{
    spdlog::trace("Intercepted VkCreateSwapchainKHR");

    auto& thisDevice = g_vulkanDevices[dispatch_key_from_handle(device)];

    // Add TRANSFER_SRC and TRANSFER_DST usages since we need to blit the image
    VkSwapchainCreateInfoKHR modifiedCreateInfo = *pCreateInfo;
    modifiedCreateInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    // Call through to create the actual swapchain
    VkResult result = thisDevice.dispatch.CreateSwapchainKHR(device, &modifiedCreateInfo, pAllocator, pSwapchain);
    if (result != VK_SUCCESS)
        return result;

    // Create and store swapchain object
    {
        std::lock_guard<std::mutex> lock(g_swapchainMutex);
        g_swapchains[*pSwapchain] = std::make_unique<vkShade::VulkanSwapchain>(thisDevice, *pSwapchain, *pCreateInfo);
    }

    spdlog::debug("Swapchain created");
    return VK_SUCCESS;
}

VK_LAYER_EXPORT void VKAPI_CALL vkShade_DestroySwapchainKHR(VkDevice                     device,
                                                            VkSwapchainKHR               swapchain,
                                                            const VkAllocationCallbacks* pAllocator)
{
    spdlog::trace("Intercepted VkDestroySwapchainKHR");

    // Clean up our bookkeeping data
    {
        std::lock_guard<std::mutex> lock(g_swapchainMutex);
        g_swapchains.erase(swapchain);
    }

    // Call through
    auto& thisDevice = g_vulkanDevices[dispatch_key_from_handle(device)];
    thisDevice.dispatch.DestroySwapchainKHR(device, swapchain, pAllocator);

    spdlog::debug("Swapchain destroyed");
}

VK_LAYER_EXPORT VkResult VKAPI_CALL vkShade_QueuePresentKHR(VkQueue queue, const VkPresentInfoKHR* pPresentInfo)
{
    // Get device
    auto& thisDevice = g_vulkanDevices[dispatch_key_from_handle(queue)];

    // Get swapchain data
    auto it = g_swapchains.find(pPresentInfo->pSwapchains[0]);
    if (it == g_swapchains.end())
    {
        spdlog::error("Swapchain not found in present");
        return thisDevice.dispatch.QueuePresentKHR(queue, pPresentInfo);
    }

    auto& swapchainData = it->second;

    // Create the GUI Manager if it doesn't exist yet
    if (!vkShade::Locator<vkShade::GuiManager>::has())
        vkShade::Locator<vkShade::GuiManager>::emplace(thisDevice, swapchainData->format());

    // Get manager handles
    auto& input = vkShade::Locator<vkShade::InputManager>::get();
    auto& gui = vkShade::Locator<vkShade::GuiManager>::get();

    // Update managers
    input.update();

    // Test input and shader loading
    if (input.is_action_just_pressed("ToggleGui"))
    {
        gui.visible(!gui.visible());
    }

    gui.update(1.f/60.f, it->second->extent());

    // For each swapchain being presented
    for (uint32_t i = 0; i < pPresentInfo->swapchainCount; i++)
    {
        VkSwapchainKHR swapchain = pPresentInfo->pSwapchains[i];
        uint32_t imageIndex = pPresentInfo->pImageIndices[i];

        auto it = g_swapchains.find(swapchain);
        if (it == g_swapchains.end())
        {
            spdlog::error("Swapchain not found in present");
            return thisDevice.dispatch.QueuePresentKHR(queue, pPresentInfo);
        }

        auto& swapchainData = it->second;

        // Render layer
        swapchainData->render(imageIndex);
    }

    // Call down the chain to present
    return thisDevice.dispatch.QueuePresentKHR(queue, pPresentInfo);
}
