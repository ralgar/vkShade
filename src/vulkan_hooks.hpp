#pragma once

#include <assert.h>
#include <mutex>
#include <unordered_map>

#include <vulkan/utility/vk_dispatch_table.h>
#include <vulkan/vulkan_core.h>

#undef VK_LAYER_EXPORT
#if defined(__GNUC__) && __GNUC__ >= 4
#define VK_LAYER_EXPORT __attribute__((visibility("default")))
#else
#error "Unsupported platform!"
#endif

// Structs to store our instances/devices and their dispatch tables
struct VulkanInstance
{
    VkInstance handle;
    VkuInstanceDispatchTable dispatch;
};

struct VulkanDevice
{
    VkDevice handle;
    VkPhysicalDevice physicalDevice;
    VkInstance instance;
    VkuDeviceDispatchTable dispatch;

    VkQueue       queue;
    uint32_t      queueFamilyIndex;
    VkCommandPool commandPool;
};

inline void* const dispatch_key_from_handle(const void* handle)
{
    assert(handle != nullptr);
    return *(void**)handle;
}

// Layer book-keeping information
// These are only modified during create/destroy
extern std::unordered_map<void*, VulkanInstance> g_vulkanInstances;
extern std::unordered_map<void*, VulkanDevice>   g_vulkanDevices;

// Single global lock, for simplicity
// Only lock when WRITING (on create and destroy)
extern std::mutex global_lock;

// Make entrypoints C-linkable
extern "C" PFN_vkVoidFunction VKAPI_CALL vkShade_GetDeviceProcAddr(VkDevice device, const char* pName);
extern "C" PFN_vkVoidFunction VKAPI_CALL vkShade_GetInstanceProcAddr(VkInstance instance, const char* pName);

// Layer init and shutdown hooks
VkResult VKAPI_CALL vkShade_CreateInstance(const VkInstanceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkInstance* pInstance);
void     VKAPI_CALL vkShade_DestroyInstance(VkInstance instance, const VkAllocationCallbacks* pAllocator);

VkResult VKAPI_CALL vkShade_CreateDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDevice* pDevice);
void     VKAPI_CALL vkShade_DestroyDevice(VkDevice device, const VkAllocationCallbacks* pAllocator);

// Swapchain hooks
VkResult VKAPI_CALL vkShade_CreateSwapchainKHR(VkDevice device, const VkSwapchainCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSwapchainKHR* pSwapchain);
void     VKAPI_CALL vkShade_DestroySwapchainKHR(VkDevice device, VkSwapchainKHR swapchain, const VkAllocationCallbacks* pAllocator);
VkResult VKAPI_CALL vkShade_QueuePresentKHR(VkQueue queue, const VkPresentInfoKHR* pPresentInfo);
