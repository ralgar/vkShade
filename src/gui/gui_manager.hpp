#pragma once

#include <vulkan/vulkan_core.h>

#include "hooks/hooks.hpp"
#include "platform/clipboard.hpp"
#include "windows/main_window.hpp"

namespace vkShade
{
    class GuiManager
    {
    public:
        GuiManager(VulkanDevice deviceContext, VkFormat swapchainFormat);
        ~GuiManager();

        // FIXME: These are useless indirection
        bool visible() { return m_mainWindow.visible(); }
        void visible(bool value) { m_mainWindow.visible(value); }

        void update(float deltaTime, VkExtent2D swapchainExtent);

    private:
        VkDevice m_device;
        VkDescriptorPool m_descriptorPool;

        std::unique_ptr<Platform::Clipboard> m_clipboard;
        MainWindow m_mainWindow;

        void draw_cursor();
    };
} // namespace vkShade
