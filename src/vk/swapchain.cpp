#include "swapchain.hpp"

#include <imgui.h>
#include <imgui_impl_vulkan.h>
#include <magic_enum/magic_enum.hpp>
#include <spdlog/spdlog.h>

#include "core/service_locator.hpp"
#include "hooks/hooks.hpp"
#include "input/input_manager.hpp"
#include "image.hpp"
#include "initializers.hpp"
#include "macros.hpp"

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

    // Create command pool
    VkCommandPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = m_device.queueFamilyIndex,
    };

    VK_CHECK(m_device.dispatch.CreateCommandPool(m_device.handle, &poolInfo, nullptr, &m_commandPool));

    // Create the fence
    VkFenceCreateInfo fenceInfo = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };

    m_device.dispatch.CreateFence(m_device.handle, &fenceInfo, nullptr, &m_fence);

    // Allocate command buffer
    VkCommandBufferAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = m_device.commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };

    VK_CHECK(m_device.dispatch.AllocateCommandBuffers(m_device.handle, &allocInfo, &m_commandBuffer));

    // Add the effects
    m_effects.push_back(std::make_shared<Effect>(m_device, m_format, "greyscale.frag.spv"));
    m_effects.push_back(std::make_shared<Effect>(m_device, m_format, "simple_grid.frag.spv"));
}

vkShade::VulkanSwapchain::~VulkanSwapchain()
{
    // Wait for the command buffer to finish executing
    m_device.dispatch.WaitForFences(m_device.handle, 1, &m_fence, true, 1000000000);

    // Clean up
    m_device.dispatch.DestroyCommandPool(m_device.handle, m_commandPool, nullptr);
    m_device.dispatch.DestroyFence(m_device.handle, m_fence, nullptr);
}

void vkShade::VulkanSwapchain::render(uint32_t imageIndex)
{
    VulkanImage* swapchainImage = m_images.at(imageIndex).get();

    // Wait until the previous command buffer has finished executing. Timeout of 1 second.
	VK_CHECK(m_device.dispatch.WaitForFences(m_device.handle, 1, &m_fence, true, 1000000000));
	VK_CHECK(m_device.dispatch.ResetFences(m_device.handle, 1, &m_fence));

    // Reset the command buffer
	VK_CHECK(m_device.dispatch.ResetCommandBuffer(m_commandBuffer, 0));

    // Begin command buffer
    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    m_device.dispatch.BeginCommandBuffer(m_commandBuffer, &beginInfo);

    // Blit swapchain image to ping-pong and transition to COLOR_ATTACHMENT
    swapchainImage->transition_layout(m_commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    m_pingPongA->transition_layout(m_commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    swapchainImage->blit_to(m_commandBuffer, m_pingPongA->image());
    m_pingPongA->transition_layout(m_commandBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    // Render effects if enabled
    auto& input = vkShade::Locator<vkShade::InputManager>::get();
    static bool enabled = true;
    if (input.is_action_just_pressed("ToggleEffects"))
        enabled = !enabled;

    VulkanImage* readImage = m_pingPongA.get();
    VulkanImage* writeImage = m_pingPongB.get();

    if (enabled)
    {
        for (auto effect : m_effects)
        {
            if (!effect)
                continue;

            // Barrier: Ensure read image is ready to sample
            VkImageMemoryBarrier2 barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
            barrier.pNext = nullptr;
            barrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
            barrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
            barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
            barrier.dstAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
            barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
            barrier.image = readImage->image();
            barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

            VkDependencyInfo depInfo{};
            depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
            depInfo.pNext = nullptr;
            depInfo.imageMemoryBarrierCount = 1;
            depInfo.pImageMemoryBarriers = &barrier;

            m_device.dispatch.CmdPipelineBarrier2(m_commandBuffer, &depInfo);

            // Bind the input (read) image
            effect->bind_input(readImage->image_view());

            // Begin dynamic rendering using our write image
            std::array<VkRenderingAttachmentInfo, 3> colorAttachments = {
                vkinit::rendering_attachment_info(writeImage->image_view(), nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL),
            };

            VkRenderingInfo renderingInfo = vkinit::rendering_info(m_extent, colorAttachments, nullptr);
            m_device.dispatch.CmdBeginRendering(m_commandBuffer, &renderingInfo);

            // Apply effect
            effect->apply(m_commandBuffer, m_extent);

            // End dynamic rendering
            m_device.dispatch.CmdEndRendering(m_commandBuffer);

            // Swap ping-pong images for next pass
            std::swap(readImage, writeImage);
        }
    }

    VulkanImage* finalImage = readImage;

    // Barrier: Ensure final image is ready to sample
    VkImageMemoryBarrier2 barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    barrier.pNext = nullptr;
    barrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    barrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
    barrier.dstAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    barrier.image = finalImage->image();
    barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

    VkDependencyInfo depInfo{};
    depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    depInfo.pNext = nullptr;
    depInfo.imageMemoryBarrierCount = 1;
    depInfo.pImageMemoryBarriers = &barrier;

    m_device.dispatch.CmdPipelineBarrier2(m_commandBuffer, &depInfo);

    // Begin dynamic rendering using our final image
    std::array<VkRenderingAttachmentInfo, 3> colorAttachments = {
        vkinit::rendering_attachment_info(finalImage->image_view(), nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL),
    };

    VkRenderingInfo renderingInfo = vkinit::rendering_info(m_extent, colorAttachments, nullptr);
    m_device.dispatch.CmdBeginRendering(m_commandBuffer, &renderingInfo);

    // Render ImGui on top of everything
    ImDrawData* imguiDrawData = ImGui::GetDrawData();
    ImGui_ImplVulkan_RenderDrawData(imguiDrawData, m_commandBuffer);

    // End dynamic rendering for ImGui
    m_device.dispatch.CmdEndRendering(m_commandBuffer);

    // Blit the final image back to swapchain and transition to PRESENT_SRC
    readImage->transition_layout(m_commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    swapchainImage->transition_layout(m_commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    readImage->blit_to(m_commandBuffer, swapchainImage->image());
    swapchainImage->transition_layout(m_commandBuffer, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    // End and submit
    m_device.dispatch.EndCommandBuffer(m_commandBuffer);

    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &m_commandBuffer,
    };

    m_device.dispatch.QueueSubmit(m_device.queue, 1, &submitInfo, m_fence);
}

vkShade::VulkanImage& vkShade::VulkanSwapchain::image(size_t index) const
{
    assert(index < m_images.size());
    return *m_images[index];
}
