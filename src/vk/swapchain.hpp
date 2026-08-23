#pragma once

#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "core/events/reload_effects.hpp"
#include "object.hpp"
#include "reshade_effect.hpp"
#include "reshade_runtime.hpp"

namespace vkShade
{
    class VulkanImage;

    class VulkanSwapchain : public VulkanObject
    {
    public:
        VulkanSwapchain(VulkanDevice& device, VkSwapchainKHR swapchain, VkSwapchainCreateInfoKHR swapchainInfo);
        ~VulkanSwapchain();

        VkExtent2D extent() const { return m_extent; }
        VkFormat format() const { return m_format; }
        VulkanImage& image(size_t index) const;
        uint32_t image_count() const { return m_images.size(); }

        void on_effects_changed(const std::string& key, std::vector<std::string> effects);

        void on_reload_effects(const Events::ReloadEffects& event);

        // Render layer on top of swapchain
        void render(uint32_t imageIndex);

    private:
        // Core swapchain resources
        VkSwapchainKHR m_swapchain;
        VkFormat m_format;
        VkExtent2D m_extent;
        std::vector<std::unique_ptr<VulkanImage>> m_images;
        std::unique_ptr<VulkanImage> m_depthDummy;

        // Layer resources
        VkFence m_fence {VK_NULL_HANDLE};
        VkCommandPool m_commandPool {VK_NULL_HANDLE};
        VkCommandBuffer m_commandBuffer {VK_NULL_HANDLE};
        std::vector<std::shared_ptr<ReshadeEffect>> m_effects;
        std::shared_ptr<VulkanImage> m_pingPongA;
        std::shared_ptr<VulkanImage> m_pingPongB;
        ReshadeRuntime m_reshadeRuntime;
    };
}  // namespace vkShade
