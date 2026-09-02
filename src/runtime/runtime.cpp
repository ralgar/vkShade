#include "runtime.hpp"

#include <imgui.h>
#include <imgui_impl_vulkan.h>
#include <magic_enum/magic_enum.hpp>
#include <string_view>

#include "config/config_manager.hpp"
#include "core/event_bus.hpp"
#include "core/logger.hpp"
#include "core/service_locator.hpp"
#include "hooks/hooks.hpp"
#include "input/input_manager.hpp"
#include "vk/image.hpp"
#include "vk/initializers.hpp"
#include "vk/macros.hpp"
#include "reshade_uniforms.hpp"

// Give the layer's render submission up to 1 second to complete. A timeout
//  indicates an abnormal GPU condition, so we will abort rather than hang.
constexpr uint64_t FENCE_TIMEOUT_NS = 1'000'000'000;

vkShade::Runtime::Runtime(VulkanDevice& device, VkSwapchainKHR swapchain, VkSwapchainCreateInfoKHR swapchainInfo)
    : VulkanObject(device)
{
    // Store the swapchain info
    m_swapchain = swapchain;
    m_format = swapchainInfo.imageFormat;
    m_extent = swapchainInfo.imageExtent;
    m_colorSpace = swapchainInfo.imageColorSpace;

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

    // Create a dummy depth image.
	VkImageUsageFlags depthImageUsages {};
	depthImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	depthImageUsages |= VK_IMAGE_USAGE_SAMPLED_BIT;

    m_depthDummy = std::make_unique<VulkanImage>(m_device, m_extent, VK_FORMAT_R32_SFLOAT, depthImageUsages);

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

    auto& preset = vkShade::Locator<vkShade::ConfigManager>::get().preset();

    // Subscribe to config changes
    preset.on_changed("", "Effects").connect<&Runtime::on_effects_changed>(this);

    // Load the current effects list
    if (auto effects = preset.get<std::vector<std::string>>("", "Effects"))
    {
        this->on_effects_changed("Effects", effects.value());
    }

    // Subscribe to events
    auto& eventBus = Locator<EventBus>::get();
    eventBus.sink<Events::ReloadEffects>().connect<&Runtime::on_reload_effects>(this);
}

vkShade::Runtime::~Runtime()
{
    // Wait for the command buffer to finish executing
    VK_CHECK(m_device.dispatch.WaitForFences(m_device.handle, 1, &m_fence, VK_TRUE, FENCE_TIMEOUT_NS));

    // Clean up
    m_device.dispatch.DestroyCommandPool(m_device.handle, m_commandPool, nullptr);
    m_device.dispatch.DestroyFence(m_device.handle, m_fence, nullptr);

    // Unsubscribe from events
    auto& eventBus = Locator<EventBus>::get();
    eventBus.sink<Events::ReloadEffects>().disconnect<&Runtime::on_reload_effects>(this);

    // Unsubscribe from config changes
    auto& preset = vkShade::Locator<vkShade::ConfigManager>::get().preset();
    preset.on_changed("", "Effects").disconnect<&Runtime::on_effects_changed>(this);
}

void vkShade::Runtime::on_effects_changed(const std::string& key, std::vector<std::string> effects)
{
    auto& config = vkShade::Locator<vkShade::ConfigManager>::get().app();
    auto searchPaths = config.get<std::vector<std::string>>("ReShade", "EffectSearchPaths");

    // Wait for the command buffer to finish executing
    VK_CHECK(m_device.dispatch.WaitForFences(m_device.handle, 1, &m_fence, VK_TRUE, FENCE_TIMEOUT_NS));

    m_effects.clear();
    std::vector<std::string> loadedEffects;
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
                            SwapchainInfo swapchainInfo = {
                                .extent = m_extent,
                                .format = m_format,
                                .colorSpace = m_colorSpace,
                            };
                            m_effects.push_back(std::make_shared<ReshadeEffect>(m_device, swapchainInfo, entry.path()));
                            loadedEffects.push_back(effect);
                            found = true;
                        } catch (const std::runtime_error& exception) {
                            Logger::error(exception.what());
                            error = true;
                        }
                        break;
                    }
                }
            }
        }

        if (!found && !error)
            Logger::warn("Unable to find effect: {}", effect);
    }

    auto& internalCfg = vkShade::Locator<vkShade::ConfigManager>::get().internal();
    internalCfg.set("__INTERNAL__", "LoadedEffects", loadedEffects);
}

void vkShade::Runtime::on_reload_effects(const Events::ReloadEffects& event)
{
    auto& preset = vkShade::Locator<vkShade::ConfigManager>::get().preset();
    auto effects = preset.get<std::vector<std::string>>("", "Effects");
    this->on_effects_changed("Effects", effects.value_or(std::vector<std::string>{}));
}

void vkShade::Runtime::render(uint32_t imageIndex)
{
    VulkanImage* swapchainImage = m_images.at(imageIndex).get();
    const auto reshadeFrameState = this->update_time();

    // Wait until the previous command buffer has finished executing.
	VK_CHECK(m_device.dispatch.WaitForFences(m_device.handle, 1, &m_fence, true, FENCE_TIMEOUT_NS));
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

    // Clear our dummy depth image and transition to SHADER_READ_ONLY
    m_depthDummy->transition_layout(m_commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    VkClearColorValue clearValue = { .float32 = { 1.0f, 0.0f, 0.0f, 0.0f } };
    VkImageSubresourceRange range = vkinit::image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT);
    m_device.dispatch.CmdClearColorImage(m_commandBuffer, m_depthDummy->image(),
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearValue, 1, &range);
    m_depthDummy->transition_layout(m_commandBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

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
            effect->update(reshadeFrameState);

            // Barrier: Ensure read image is ready to sample
            readImage->transition_layout(m_commandBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

            // Bind the color and depth images
            effect->bind_input(*readImage, *m_depthDummy);

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
    m_frameCount++;
}

vkShade::VulkanImage& vkShade::Runtime::image(size_t index) const
{
    assert(index < m_images.size());
    return *m_images[index];
}

vkShade::ReshadeFrameState vkShade::Runtime::update_time()
{
    ReshadeFrameState state {};

    Clock::time_point now = Clock::now();

    state.frameTime = reshade_frame_time(now - m_lastFrame);
    state.frameCount = static_cast<uint32_t>(m_frameCount % std::numeric_limits<uint32_t>::max());
    state.timer = reshade_frame_time(now - m_start);

    m_lastFrame = now;

    return state;
}
