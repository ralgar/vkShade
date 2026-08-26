#pragma once

#include <vulkan/vulkan_core.h>

#include "hooks/hooks.hpp"
#include "windows/main_window.hpp"

namespace vkShade
{
    struct KeyboardEvent;
    struct MouseButtonEvent;
    struct MouseMotionEvent;
    struct MouseWheelEvent;
    struct TextInputEvent;

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

        MainWindow m_mainWindow;

        void draw_cursor();

        void on_keyboard_event(const KeyboardEvent& event);
        void on_mouse_button_event(const MouseButtonEvent& event);
        void on_mouse_motion_event(const MouseMotionEvent& event);
        void on_mouse_wheel_event(const MouseWheelEvent& event);
        void on_text_input_event(const TextInputEvent& event);
    };
} // namespace vkShade
