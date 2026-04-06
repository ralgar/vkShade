#include "gui_manager.hpp"

#include <imgui.h>
#include <imgui_impl_vulkan.h>
#include <vulkan/vulkan_core.h>

#include "core/service_locator.hpp"
#include "hooks/hooks.hpp"
#include "input/input_manager.hpp"
#include "windows/main_window.hpp"
#include "vk/macros.hpp"
#include "gui_style.hpp"

vkShade::GuiManager::GuiManager(VulkanDevice deviceContext, VkFormat swapchainFormat)
{
    m_device = deviceContext.handle;

	// 1: Create descriptor pool for ImGui.
	// This pool is very oversized, but it's copied from ImGui demo itself.
	VkDescriptorPoolSize pool_sizes[] = { { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 } };

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	pool_info.maxSets = 1000;
	pool_info.poolSizeCount = (uint32_t)std::size(pool_sizes);
	pool_info.pPoolSizes = pool_sizes;

	VK_CHECK(deviceContext.dispatch.CreateDescriptorPool(m_device, &pool_info, nullptr, &m_descriptorPool));

	// 2: Initialize ImGui library.

	// This initializes the core structures of ImGui.
	ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

	// This initializes ImGui for Vulkan.
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = deviceContext.instance;
	init_info.PhysicalDevice = deviceContext.physicalDevice;
	init_info.Device = deviceContext.handle;
	init_info.Queue = deviceContext.queue;
    init_info.QueueFamily = deviceContext.queueFamilyIndex;
	init_info.DescriptorPool = m_descriptorPool;
	init_info.MinImageCount = 3;
	init_info.ImageCount = 3;
	init_info.UseDynamicRendering = true;

    // Custom loader that uses the dispatch table
    ImGui_ImplVulkan_LoadFunctions([](const char* function_name, void* user_data) -> PFN_vkVoidFunction
    {
        VulkanDevice* ctx = (VulkanDevice*)user_data;
        auto& instance = get_instance_from_handle(ctx->instance);

        // Try the device table first (most functions are device-level)
        PFN_vkVoidFunction func = vkShade_GetDeviceProcAddr(ctx->handle, function_name);
        if (func)
        {
            spdlog::trace("[ImGui] Loaded {} from device table:  {}", function_name, (void*)func);
            return func;
        }

        // Fall back to the instance table
        func = vkShade_GetInstanceProcAddr(instance.handle, function_name);
        if (func)
        {
            spdlog::trace("[ImGui] Loaded {} from instance table: {}", function_name, (void*)func);
            return func;
        }

        spdlog::error("[ImGui] Failed to load function: {}", function_name);
        return nullptr;
    }, &deviceContext);

	// Dynamic rendering parameters for ImGui to use.
	init_info.PipelineRenderingCreateInfo = {.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
	init_info.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
	init_info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &swapchainFormat;

	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

	ImGui_ImplVulkan_Init(&init_info);
	ImGui_ImplVulkan_CreateFontsTexture();

    // Apply custom style
    UIStyle::ApplyStyle();
}

vkShade::GuiManager::~GuiManager()
{
    ImGui_ImplVulkan_Shutdown();
    auto& thisDevice = get_device_from_handle(m_device);
    thisDevice.dispatch.DestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
}

void vkShade::GuiManager::update(float deltaTime, VkExtent2D swapchainExtent)
{
    auto& input = vkShade::Locator<InputManager>::get();

    ImGuiIO& io = ImGui::GetIO();

    // Update mouse state
    glm::vec2 mousePos = input.mouse_position();
    io.AddMousePosEvent(mousePos.x, mousePos.y);
    io.AddMouseButtonEvent(0, input.is_mouse_button_pressed(MouseButton::LEFT));
    io.AddMouseButtonEvent(1, input.is_mouse_button_pressed(MouseButton::RIGHT));
    io.AddMouseButtonEvent(2, input.is_mouse_button_pressed(MouseButton::MIDDLE));

    io.DisplaySize = ImVec2((float)swapchainExtent.width, (float)swapchainExtent.height);
    io.DeltaTime = deltaTime;

    ImGui::NewFrame();

    // Test window
    if (m_mainWindow.visible())
    {
        m_mainWindow.render();

        // Last: Draw a black cursor with white outline and red center dot
        ImDrawList* draw_list = ImGui::GetForegroundDrawList();
        ImVec2 pos(mousePos.x, mousePos.y);
        draw_list->AddCircleFilled(pos, 4.0f, IM_COL32(0, 0, 0, 255));
        draw_list->AddCircle(pos, 5.0f, IM_COL32(255, 255, 255, 255), 0, 1.0f);
        draw_list->AddCircleFilled(pos, 1.5f, IM_COL32(255, 0, 0, 255));
    }

    ImGui::Render();
}
