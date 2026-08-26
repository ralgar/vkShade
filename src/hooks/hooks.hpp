#pragma once

#include <assert.h>
#include <memory>
#include <shared_mutex>
#include <unordered_map>

#include <vk_mem_alloc.h>
#include <vulkan/utility/vk_dispatch_table.h>
#include <vulkan/vulkan_core.h>

#include "core/diagnostics_state.hpp"
#include "vk/image_tracker.hpp"

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
    const uint32_t apiVersion;
    VkuInstanceDispatchTable dispatch;
};

struct VulkanDevice
{
    VkDevice handle;
    VkPhysicalDevice physicalDevice;
    VkInstance instance;
    VkuDeviceDispatchTable dispatch;
    VmaAllocator allocator;

    VkQueue       queue;
    uint32_t      queueFamilyIndex;
    uint32_t      timestampValidBits {0};
    float         timestampPeriod {0.0f};
    VkCommandPool commandPool;
    std::shared_ptr<vkShade::DiagnosticsState> diagnosticsState;
    std::shared_ptr<vkShade::ImageTracker> imageTracker;
};

inline void* const dispatch_key_from_handle(const void* handle)
{
    // Vulkan handles are pointers to dispatch tables - dereference to get the key
    assert(handle != nullptr);
    return *reinterpret_cast<void* const*>(handle);
}

// Layer book-keeping information
// These are only modified during create/destroy
extern std::unordered_map<void*, VulkanInstance> g_vulkanInstances;
extern std::unordered_map<void*, VulkanDevice>   g_vulkanDevices;

// Convenience functions for getting handles
inline VulkanInstance& get_instance_from_handle(const void* handle)
{
    auto it = g_vulkanInstances.find(dispatch_key_from_handle(handle));
    assert(it != g_vulkanInstances.end() && "Invalid instance key");
    return it->second;
}

inline VulkanDevice& get_device_from_handle(const void* handle)
{
    auto it = g_vulkanDevices.find(dispatch_key_from_handle(handle));
    assert(it != g_vulkanDevices.end() && "Invalid device key");
    return it->second;
}

// Reader/writer lock for concurrent read access to bookkeeping maps.
extern std::shared_mutex g_globalLock;

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

// Command Buffer hooks
VkResult VKAPI_CALL vkShade_AllocateCommandBuffers(VkDevice                           device,
                                                   const VkCommandBufferAllocateInfo* pAllocateInfo,
                                                   VkCommandBuffer*                   pCommandBuffers);

VkResult VKAPI_CALL vkShade_CreateImage(VkDevice device, const VkImageCreateInfo* pCreateInfo,
                                        const VkAllocationCallbacks* pAllocator, VkImage* pImage);
void VKAPI_CALL vkShade_DestroyImage(VkDevice device, VkImage image,
                                     const VkAllocationCallbacks* pAllocator);
VkResult VKAPI_CALL vkShade_CreateImageView(VkDevice device,
                                            const VkImageViewCreateInfo* pCreateInfo,
                                            const VkAllocationCallbacks* pAllocator,
                                            VkImageView* pView);
void VKAPI_CALL vkShade_DestroyImageView(VkDevice device, VkImageView imageView,
                                         const VkAllocationCallbacks* pAllocator);
VkResult VKAPI_CALL vkShade_CreateFramebuffer(VkDevice device,
                                              const VkFramebufferCreateInfo* pCreateInfo,
                                              const VkAllocationCallbacks* pAllocator,
                                              VkFramebuffer* pFramebuffer);
void VKAPI_CALL vkShade_DestroyFramebuffer(VkDevice device, VkFramebuffer framebuffer,
                                           const VkAllocationCallbacks* pAllocator);
void VKAPI_CALL vkShade_CmdBeginRendering(VkCommandBuffer commandBuffer,
                                          const VkRenderingInfo* pRenderingInfo);
void VKAPI_CALL vkShade_CmdBeginRenderingKHR(VkCommandBuffer commandBuffer,
                                             const VkRenderingInfo* pRenderingInfo);
void VKAPI_CALL vkShade_CmdBeginRenderPass(VkCommandBuffer commandBuffer,
                                           const VkRenderPassBeginInfo* pRenderPassBegin,
                                           VkSubpassContents contents);
void VKAPI_CALL vkShade_CmdBeginRenderPass2(VkCommandBuffer commandBuffer,
                                            const VkRenderPassBeginInfo* pRenderPassBegin,
                                            const VkSubpassBeginInfo* pSubpassBeginInfo);
void VKAPI_CALL vkShade_CmdBeginRenderPass2KHR(VkCommandBuffer commandBuffer,
                                               const VkRenderPassBeginInfo* pRenderPassBegin,
                                               const VkSubpassBeginInfo* pSubpassBeginInfo);
