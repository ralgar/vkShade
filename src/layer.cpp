#include "vulkan_hooks.hpp"

#include <string.h>
#include <mutex>

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

    {
        std::lock_guard<std::mutex> lock(global_lock);

        auto& thisDevice = g_vulkanDevices[dispatch_key_from_handle(device)];
        return thisDevice.dispatch.GetDeviceProcAddr(device, pName);
    }
}

VK_LAYER_EXPORT PFN_vkVoidFunction VKAPI_CALL vkShade_GetInstanceProcAddr(VkInstance instance, const char* pName)
{
    // Self-intercept here as well to stay consistent with 'vkShade_GetDeviceProcAddr' implementation
    GETPROCADDR(GetInstanceProcAddr);

    // Core instance-chain functions
    GETPROCADDR(CreateInstance);
    GETPROCADDR(DestroyInstance);
    GETPROCADDR(CreateDevice);
    GETPROCADDR(DestroyDevice);

    {
        std::lock_guard<std::mutex> lock(global_lock);

        auto& thisInstance = g_vulkanInstances[dispatch_key_from_handle(instance)];
        return thisInstance.dispatch.GetInstanceProcAddr(instance, pName);
    }
}
