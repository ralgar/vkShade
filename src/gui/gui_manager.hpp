#pragma once

#include <vulkan/vulkan_core.h>

#include "hooks/hooks.hpp"

namespace vkShade
{
    class GuiManager
    {
    public:
        GuiManager(VulkanDevice deviceContext, VkFormat swapchainFormat);
        ~GuiManager();

        bool visible() { return m_visible; }
        void visible(bool value) { m_visible = value; }

        void update(float deltaTime, VkExtent2D swapchainExtent);

    private:
        VkDevice m_device;
        VkDescriptorPool m_descriptorPool;

        bool m_visible {false};
    };
} // namespace vkShade
