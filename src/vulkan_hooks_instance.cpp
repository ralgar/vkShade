#include "vulkan_hooks.hpp"

#include <vulkan/vk_layer.h>

#include <stdio.h>

VK_LAYER_EXPORT VkResult VKAPI_CALL vkShade_CreateInstance(
    const VkInstanceCreateInfo*                 pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkInstance*                                 pInstance)
{
    // Step through the pNext chain until we get to the layer link info
    VkLayerInstanceCreateInfo* layerInfo = (VkLayerInstanceCreateInfo*)pCreateInfo->pNext;
    while(layerInfo && (layerInfo->sType != VK_STRUCTURE_TYPE_LOADER_INSTANCE_CREATE_INFO || layerInfo->function != VK_LAYER_LINK_INFO))
    {
        layerInfo = (VkLayerInstanceCreateInfo*)layerInfo->pNext;
    }

    if(!layerInfo)
    {
        fprintf(stderr, "[Layer] Failed to find instance layer link info\n");
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    // Get the next layer's vkGetInstanceProcAddr
    PFN_vkGetInstanceProcAddr gpa = layerInfo->u.pLayerInfo->pfnNextGetInstanceProcAddr;
    layerInfo->u.pLayerInfo = layerInfo->u.pLayerInfo->pNext;  // Move chain on for next layer

    // Get vkCreateInstance from the next layer
    PFN_vkCreateInstance create_func = (PFN_vkCreateInstance)gpa(VK_NULL_HANDLE, "vkCreateInstance");
    if (!create_func)
    {
        fprintf(stderr, "[Layer] Failed to get vkCreateInstance\n");
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    // Call through to next layer
    VkResult result = create_func(pCreateInfo, pAllocator, pInstance);
    if (result != VK_SUCCESS)
    {
        return result;
    }

    // Create our instance data
    VulkanInstanceData data;
    data.instance = *pInstance;

    // Initialize dispatch table
    data.dispatchTable.GetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)gpa(*pInstance, "vkGetInstanceProcAddr");
    data.dispatchTable.DestroyInstance = (PFN_vkDestroyInstance)gpa(*pInstance, "vkDestroyInstance");

    // Store instance data
    {
        std::lock_guard<std::mutex> lock(global_lock);
        g_vulkanInstances[dispatch_key_from_handle(*pInstance)] = data;
    }

    return VK_SUCCESS;
}

VK_LAYER_EXPORT void VKAPI_CALL vkShade_DestroyInstance(VkInstance instance, const VkAllocationCallbacks* pAllocator)
{
    std::lock_guard<std::mutex> lock(global_lock);
    g_vulkanInstances.erase(dispatch_key_from_handle(instance));
}
