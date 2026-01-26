#include "vulkan_hooks.hpp"

#include <vulkan/vk_layer.h>

#include <stdio.h>

VK_LAYER_EXPORT VkResult VKAPI_CALL vkShade_CreateDevice(
    VkPhysicalDevice                            physicalDevice,
    const VkDeviceCreateInfo*                   pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkDevice*                                   pDevice)
{
    // Step through the pNext chain until we get to the layer link info
    VkLayerDeviceCreateInfo* layerInfo = (VkLayerDeviceCreateInfo*)pCreateInfo->pNext;
    while(layerInfo && (layerInfo->sType != VK_STRUCTURE_TYPE_LOADER_DEVICE_CREATE_INFO || layerInfo->function != VK_LAYER_LINK_INFO))
    {
        layerInfo = (VkLayerDeviceCreateInfo *)layerInfo->pNext;
    }

    if(!layerInfo)
    {
        // No loader instance create info
        fprintf(stderr, "[Layer] Failed to find device layer link info\n");
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    // Get the next layer's proc addrs
    PFN_vkGetInstanceProcAddr gipa = layerInfo->u.pLayerInfo->pfnNextGetInstanceProcAddr;
    PFN_vkGetDeviceProcAddr gdpa = layerInfo->u.pLayerInfo->pfnNextGetDeviceProcAddr;
    layerInfo->u.pLayerInfo = layerInfo->u.pLayerInfo->pNext;  // Move chain on for next layer

    // Get vkCreateDevice from next layer
    PFN_vkCreateDevice create_func = (PFN_vkCreateDevice)gipa(VK_NULL_HANDLE, "vkCreateDevice");
    if (!create_func)
    {
        fprintf(stderr, "[Layer] Failed to get vkCreateDevice\n");
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    // Call through to next layer
    VkResult result = create_func(physicalDevice, pCreateInfo, pAllocator, pDevice);
    if (result != VK_SUCCESS)
    {
        return result;
    }

    // Create device data
    VulkanDeviceData data;
    data.device = *pDevice;
    data.physicalDevice = physicalDevice;
    //data.instance = g_physdev_to_instance[dispatch_key_from_handle(physicalDevice)];  // TODO: Set this

    // Initialize dispatch table
    data.dispatchTable.GetDeviceProcAddr = (PFN_vkGetDeviceProcAddr)gdpa(*pDevice, "vkGetDeviceProcAddr");
    data.dispatchTable.DestroyDevice = (PFN_vkDestroyDevice)gdpa(*pDevice, "vkDestroyDevice");

    // Swapchain hooks
    data.dispatchTable.QueuePresentKHR = (PFN_vkQueuePresentKHR)gdpa(*pDevice, "vkQueuePresentKHR");

    // Store device data
    {
        std::lock_guard<std::mutex> lock(global_lock);
        g_vulkanDevices[dispatch_key_from_handle(*pDevice)] = data;
    }

    return VK_SUCCESS;
}

VK_LAYER_EXPORT void VKAPI_CALL vkShade_DestroyDevice(VkDevice device, const VkAllocationCallbacks* pAllocator)
{
    std::lock_guard<std::mutex> lock(global_lock);
    g_vulkanDevices.erase(dispatch_key_from_handle(device));
}
