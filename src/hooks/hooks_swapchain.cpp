#include "gui/gui_manager.hpp"
#include "hooks.hpp"
#include "vk/swapchain.hpp"

#include <imgui.h>
#include <imgui_impl_vulkan.h>
#include <vulkan/vulkan_core.h>
#include <spdlog/spdlog.h>

#include "core/service_locator.hpp"
#include "input/input_manager.hpp"

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
    vkShade::VulkanSwapchain swapchain(thisDevice.handle, *pSwapchain, *pCreateInfo);
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

    // Get manager handles
    auto& input = vkShade::Locator<vkShade::InputManager>::get();
    auto& gui = vkShade::Locator<vkShade::GuiManager>::get();

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
        VkImage image = swapchainData.image(imageIndex);
        VkDevice device = swapchainData.device();

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

        // Transition to COLOR_ATTACHMENT_OPTIMAL for rendering
        VkImageMemoryBarrier barrier1 = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = 0,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = image,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        };
        thisDevice.dispatch.CmdPipelineBarrier(
            cmd,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            0, 0, nullptr, 0, nullptr, 1, &barrier1
        );

        // Begin dynamic rendering
        VkRenderingAttachmentInfo colorAttachment = {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = swapchainData.image_view(imageIndex),  // You need to track image views
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

        // Draw ImGui
        thisDevice.dispatch.CmdBeginRendering(cmd, &renderingInfo);

        ImDrawData* draw_data = ImGui::GetDrawData();
        ImGui_ImplVulkan_RenderDrawData(draw_data, cmd);

        thisDevice.dispatch.CmdEndRendering(cmd);

        // Transition back to PRESENT_SRC
        VkImageMemoryBarrier barrier2 = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .dstAccessMask = 0,
            .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = image,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        };
        thisDevice.dispatch.CmdPipelineBarrier(
            cmd,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            0, 0, nullptr, 0, nullptr, 1, &barrier2
        );

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

    // Present normally
    return thisDevice.dispatch.QueuePresentKHR(queue, pPresentInfo);
}
