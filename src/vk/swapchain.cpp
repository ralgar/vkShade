#include "swapchain.hpp"

#include <imgui.h>
#include <imgui_impl_vulkan.h>
#include <magic_enum/magic_enum.hpp>
#include <spdlog/spdlog.h>

#include "config/config_manager.hpp"
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
        .commandPool = m_commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };

    // NOTE: Command buffers are dispatchable types. Their creation must go through our
    //  own hook, not the dispatch table, to ensure the dispatch pointer fixup runs.
    VK_CHECK(vkShade_AllocateCommandBuffers(m_device.handle, &allocInfo, &m_commandBuffer));

    auto& config = vkShade::Locator<vkShade::ConfigManager>::get().app();

    // Subscribe to config changes
    config.on_changed("vkShade", "Effects").connect<&VulkanSwapchain::on_effects_changed>(this);

    // Load the current effects list
    if (auto effects = config.get<std::vector<std::string>>("vkShade", "Effects"))
    {
        this->on_effects_changed(effects.value());
    }
}

vkShade::VulkanSwapchain::~VulkanSwapchain()
{
    // Wait for the command buffer to finish executing
    m_device.dispatch.WaitForFences(m_device.handle, 1, &m_fence, true, 1000000000);

    // Clean up
    m_device.dispatch.DestroyCommandPool(m_device.handle, m_commandPool, nullptr);
    m_device.dispatch.DestroyFence(m_device.handle, m_fence, nullptr);

    // Unsubscribe from config changes
    auto& config = vkShade::Locator<vkShade::ConfigManager>::get().app();
    config.on_changed("vkShade", "Effects").disconnect<&VulkanSwapchain::on_effects_changed>(this);
}

void vkShade::VulkanSwapchain::on_effects_changed(std::vector<std::string> effects)
{
    auto& config = vkShade::Locator<vkShade::ConfigManager>::get().app();
    auto searchPaths = config.get<std::vector<std::string>>("ReShade", "EffectSearchPaths");

    m_effects.clear();
    for (const auto& effect : effects)
    {
        bool found = false;
        bool error = false;
        for (const auto& path : searchPaths.value_or(std::vector<std::string>{}))
        {
            if (std::filesystem::exists(path) && std::filesystem::is_directory(path))
            {
                for (const auto& entry : std::filesystem::directory_iterator(path))
                {
                    if (entry.path().filename() == effect)
                    {
                        try {
                            m_effects.push_back(std::make_shared<ReshadeEffect>(m_device, m_extent, m_format, entry.path()));
                            found = true;
                        } catch (const std::runtime_error& exception) {
                            spdlog::error(exception.what());
                            error = true;
                        }
                        break;
                    }
                }
            }
        }

        if (!found && !error)
            spdlog::warn("Unable to find effect: {}", effect);
    }
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

            // Update the effect uniforms
            effect->update();

            // Barrier: Ensure read image is ready to sample
            readImage->transition_layout(m_commandBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

            // Bind the input (read) image
            effect->bind_input(readImage->image_view());

            // Apply effect
            effect->apply(m_commandBuffer, *writeImage);

            // Swap ping-pong images for next pass
            std::swap(readImage, writeImage);
        }
    }

    VulkanImage* finalImage = readImage;

    // Barrier: Ensure final image is ready to sample
    finalImage->transition_layout(m_commandBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    // Begin dynamic rendering using our final image
    std::array<VkRenderingAttachmentInfo, 1> colorAttachments = {
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
