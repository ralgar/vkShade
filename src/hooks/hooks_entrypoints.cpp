#include "hooks/hooks.hpp"
#include "hooks/hooks_surface.hpp"

#include <string.h>
#include <vulkan/vulkan_core.h>

// Layer book-keeping information
// These are only modified during create/destroy
// NOTE: These are declared in `hooks.hpp`
std::unordered_map<void*, VulkanInstance> g_vulkanInstances;
std::unordered_map<void*, VulkanDevice>   g_vulkanDevices;

// Reader/writer lock for concurrent read access to bookkeeping maps.
// NOTE: This is declared in `hooks.hpp`
std::shared_mutex g_globalLock;

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
    GETPROCADDR(CreateImage);
    GETPROCADDR(DestroyImage);
    GETPROCADDR(CreateImageView);
    GETPROCADDR(DestroyImageView);
    GETPROCADDR(CreateFramebuffer);
    GETPROCADDR(DestroyFramebuffer);
    GETPROCADDR(CmdBeginRendering);
    GETPROCADDR(CmdBeginRenderingKHR);
    GETPROCADDR(CmdBeginRenderPass);
    GETPROCADDR(CmdBeginRenderPass2);
    GETPROCADDR(CmdBeginRenderPass2KHR);

    // Acquire a reader lock
    {
        std::shared_lock lock(g_globalLock);
        auto& thisDevice = get_device_from_handle(device);
        return reinterpret_cast<PFN_vkVoidFunction>(thisDevice.dispatch.GetDeviceProcAddr(device, pName));
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

    // Surface functions
    GETPROCADDR(CreateWaylandSurfaceKHR);
    GETPROCADDR(CreateXcbSurfaceKHR);
    GETPROCADDR(CreateXlibSurfaceKHR);

    // Acquire a reader lock
    {
        std::shared_lock lock(g_globalLock);
        auto& thisInstance = get_instance_from_handle(instance);
        return reinterpret_cast<PFN_vkVoidFunction>(thisInstance.dispatch.GetInstanceProcAddr(instance, pName));
    }
}
