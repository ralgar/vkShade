#include "hooks/hooks.hpp"
#include "hooks/hooks_surface.hpp"

#include <string.h>
#include <mutex>
#include <vulkan/vulkan_core.h>

#include "config/config_manager.hpp"
#include "core/service_locator.hpp"

// Layer book-keeping information
// These are only modified during create/destroy
// NOTE: These are declared in `vulkan_hooks.hpp`
std::unordered_map<void*, VulkanInstance> g_vulkanInstances;
std::unordered_map<void*, VulkanDevice>   g_vulkanDevices;

// Single global lock, for simplicity
// Only lock when WRITING (on create and destroy)
// NOTE: This is declared in `vulkan_hooks.hpp`
std::mutex global_lock;

///////////////////////////////////////////////////////////////////////////////////////////
// GetProcAddr functions, the entry points of the layer.
// Here we define what Vulkan functions we actually want to hook into.
///////////////////////////////////////////////////////////////////////////////////////////

#define GETPROCADDR(func) if(!strcmp(pName, "vk" #func)) return (PFN_vkVoidFunction)&vkShade_##func;

VK_LAYER_EXPORT PFN_vkVoidFunction VKAPI_CALL vkShade_GetDeviceProcAddr(VkDevice device, const char* pName)
{
	// Need to self-intercept, since some layers rely on this (e.g. Steam overlay)
	// See https://github.com/KhronosGroup/Vulkan-Loader/blob/master/loader/LoaderAndLayerInterface.md#layer-conventions-and-rules
    GETPROCADDR(GetDeviceProcAddr);

    // Core device-chain functions
    GETPROCADDR(DestroyDevice);

    // Device chain functions we intercept
    GETPROCADDR(CreateSwapchainKHR);
    GETPROCADDR(DestroySwapchainKHR);
    GETPROCADDR(QueuePresentKHR);

    GETPROCADDR(AllocateCommandBuffers);

    {
        std::lock_guard<std::mutex> lock(global_lock);

        auto& thisDevice = get_device_from_handle(device);
        return reinterpret_cast<PFN_vkVoidFunction>(thisDevice.dispatch.GetDeviceProcAddr(device, pName));
    }
}

VK_LAYER_EXPORT PFN_vkVoidFunction VKAPI_CALL vkShade_GetInstanceProcAddr(VkInstance instance, const char* pName)
{
    vkShade::Locator<vkShade::ConfigManager>::emplace().app();

    // Self-intercept here as well to stay consistent with 'vkShade_GetDeviceProcAddr' implementation
    GETPROCADDR(GetInstanceProcAddr);

    // Core instance-chain functions
    GETPROCADDR(CreateInstance);
    GETPROCADDR(DestroyInstance);
    GETPROCADDR(CreateDevice);
    GETPROCADDR(DestroyDevice);

    // Surface functions
    GETPROCADDR(CreateWaylandSurfaceKHR);
    GETPROCADDR(CreateXcbSurfaceKHR);
    GETPROCADDR(CreateXlibSurfaceKHR);

    {
        std::lock_guard<std::mutex> lock(global_lock);

        auto& thisInstance = get_instance_from_handle(instance);
        return reinterpret_cast<PFN_vkVoidFunction>(thisInstance.dispatch.GetInstanceProcAddr(instance, pName));
    }
}
