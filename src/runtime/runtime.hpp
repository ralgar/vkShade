#pragma once

#include <chrono>
#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "core/events/reload_effects.hpp"
#include "vk/object.hpp"
#include "reshade_effect.hpp"

namespace vkShade
{
    class VulkanImage;

    class Runtime : public VulkanObject
    {
    public:
        Runtime(VulkanDevice& device, VkSwapchainKHR swapchain, VkSwapchainCreateInfoKHR swapchainInfo);
        ~Runtime();

        VkExtent2D extent() const { return m_extent; }
        VkFormat format() const { return m_format; }
        VulkanImage& image(size_t index) const;
        uint32_t image_count() const { return m_images.size(); }

        void on_effects_changed(const std::string& key, std::vector<std::string> effects);

        void on_reload_effects(const Events::ReloadEffects& event);

        // Render layer on top of swapchain
        void render(uint32_t imageIndex);

    private:
        using Clock = std::chrono::steady_clock;

        // Swapchain resources
        VkSwapchainKHR m_swapchain;
        VkFormat m_format;
        VkExtent2D m_extent;
        VkColorSpaceKHR m_colorSpace;
        std::vector<std::unique_ptr<VulkanImage>> m_images;
        std::unique_ptr<VulkanImage> m_depthDummy;

        // Rendering resources
        VkFence m_fence {VK_NULL_HANDLE};
        VkCommandPool m_commandPool {VK_NULL_HANDLE};
        VkCommandBuffer m_commandBuffer {VK_NULL_HANDLE};
        std::vector<std::shared_ptr<ReshadeEffect>> m_effects;
        std::shared_ptr<VulkanImage> m_pingPongA;
        std::shared_ptr<VulkanImage> m_pingPongB;

        // ReShade runtime uniform state
        Clock::time_point m_start = Clock::now();
        Clock::time_point m_lastFrame = m_start;
        uint64_t m_frameCount {0};

        [[nodiscard]]
        ReshadeFrameState update_time();
    };
}  // namespace vkShade
