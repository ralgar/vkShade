#include "gui/gui_manager.hpp"
#include "hooks.hpp"
#include "vk/swapchain.hpp"

#include <imgui.h>
#include <imgui_impl_vulkan.h>
#include <vulkan/vulkan_core.h>
#include <spdlog/spdlog.h>

#include "core/service_locator.hpp"
#include "input/input_manager.hpp"
#include "vk/effect.hpp"
#include "vk/image.hpp"

std::unordered_map<VkSwapchainKHR, vkShade::VulkanSwapchain> g_swapchains;
std::mutex g_swapchainMutex;

VK_LAYER_EXPORT VkResult VKAPI_CALL vkShade_CreateSwapchainKHR(VkDevice                        device,
                                                               const VkSwapchainCreateInfoKHR* pCreateInfo,
                                                               const VkAllocationCallbacks*    pAllocator,
                                                               VkSwapchainKHR*                 pSwapchain)
{
    spdlog::trace("Intercepted VkCreateSwapchainKHR");

    auto& thisDevice = g_vulkanDevices[dispatch_key_from_handle(device)];

    // Call through to create the actual swapchain
    VkResult result = thisDevice.dispatch.CreateSwapchainKHR(device, pCreateInfo, pAllocator, pSwapchain);
    if (result != VK_SUCCESS)
        return result;

    // Create and store swapchain object
    vkShade::VulkanSwapchain swapchain(thisDevice, *pSwapchain, *pCreateInfo);
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
        vkShade::Locator<vkShade::GuiManager>::emplace(thisDevice, swapchainData.format());

    // Create the Effect if it doesn't exist yet
    if (!vkShade::Locator<vkShade::Effect>::has())
        vkShade::Locator<vkShade::Effect>::emplace(thisDevice, swapchainData.format());

    // Get manager handles
    auto& input = vkShade::Locator<vkShade::InputManager>::get();
    auto& gui = vkShade::Locator<vkShade::GuiManager>::get();
    auto& effect = vkShade::Locator<vkShade::Effect>::get();

    // Update managers
    input.update();

    // Test input and shader loading
    if (input.is_action_just_pressed("TestAction"))
    {
        gui.visible(!gui.visible());
    }

    gui.update(1.f/60.f, it->second.extent());

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
        vkShade::VulkanImage& swapchainImage = swapchainData.image(imageIndex);
        vkShade::VulkanImage& pingPongA = swapchainData.ping_pong_a();
        VkDevice device = swapchainData.device().handle;

        // Allocate command buffer
        VkCommandBuffer cmd;
        VkCommandBufferAllocateInfo allocInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = thisDevice.commandPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };
        thisDevice.dispatch.AllocateCommandBuffers(device, &allocInfo, &cmd);

        // Begin command buffer
        VkCommandBufferBeginInfo beginInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        };
        thisDevice.dispatch.BeginCommandBuffer(cmd, &beginInfo);

        // Blit swapchain image to ping-pong and transition to COLOR_ATTACHMENT
        swapchainImage.transition_layout(cmd, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        pingPongA.transition_layout(cmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        swapchainImage.blit_to(cmd, pingPongA.image());
        pingPongA.transition_layout(cmd, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        // Begin dynamic rendering
        VkRenderingAttachmentInfo colorAttachment = {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = pingPongA.image_view(),
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,  // Keep existing content
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        };

        VkRenderingInfo renderingInfo = {
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .renderArea = {.extent = swapchainData.extent()},
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &colorAttachment,
        };

        // Begin dynamic rendering
        thisDevice.dispatch.CmdBeginRendering(cmd, &renderingInfo);

        // Render effect
        effect.bind_input(swapchainImage.image_view());
        effect.render(cmd, swapchainData.extent());

        // Render ImGui on top of everything
        ImDrawData* draw_data = ImGui::GetDrawData();
        ImGui_ImplVulkan_RenderDrawData(draw_data, cmd);

        // End dynamic rendering
        thisDevice.dispatch.CmdEndRendering(cmd);

        // Blit ping-pong image back to swapchain and transition to PRESENT_SRC
        pingPongA.transition_layout(cmd, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        swapchainImage.transition_layout(cmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        pingPongA.blit_to(cmd, swapchainImage.image());
        swapchainImage.transition_layout(cmd, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

        // End and submit
        thisDevice.dispatch.EndCommandBuffer(cmd);

        VkSubmitInfo submitInfo = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &cmd,
        };
        thisDevice.dispatch.QueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
        thisDevice.dispatch.QueueWaitIdle(queue);

        // Free command buffer
        thisDevice.dispatch.FreeCommandBuffers(device, thisDevice.commandPool, 1, &cmd);
    }

    // Call down the chain to present
    return thisDevice.dispatch.QueuePresentKHR(queue, pPresentInfo);
}
