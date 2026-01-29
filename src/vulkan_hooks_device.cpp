#include "vulkan_hooks.hpp"

#include <magic_enum/magic_enum.hpp>
#include <spdlog/spdlog.h>
#include <vulkan/vk_layer.h>

#include "core/service_locator.hpp"
#include "core/resource_cache.hpp"
#include "vulkan_shader_module.hpp"

VK_LAYER_EXPORT VkResult VKAPI_CALL vkShade_CreateDevice(
    VkPhysicalDevice                            physicalDevice,
    const VkDeviceCreateInfo*                   pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkDevice*                                   pDevice)
{
    spdlog::trace("Intercepted VkCreateDevice");

    // Step through the pNext chain until we get to the layer link info
    VkLayerDeviceCreateInfo* layerInfo = (VkLayerDeviceCreateInfo*)pCreateInfo->pNext;
    while(layerInfo && (layerInfo->sType != VK_STRUCTURE_TYPE_LOADER_DEVICE_CREATE_INFO || layerInfo->function != VK_LAYER_LINK_INFO))
    {
        layerInfo = (VkLayerDeviceCreateInfo *)layerInfo->pNext;
    }

    if(!layerInfo)
    {
        spdlog::error("Failed to find device layer link info");
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
        spdlog::error("Failed to get vkCreateDevice");
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    // Call through to next layer
    VkResult result = create_func(physicalDevice, pCreateInfo, pAllocator, pDevice);
    if (result != VK_SUCCESS)
    {
        spdlog::error("Failed to create device: {}", magic_enum::enum_name(result));
        return result;
    }

    // Create device data
    VulkanDevice thisDevice;
    thisDevice.handle = *pDevice;
    thisDevice.physicalDevice = physicalDevice;
    //thisDevice.instance = g_physdev_to_instance[dispatch_key_from_handle(physicalDevice)];  // TODO: Set this

    // Initialize dispatch table
    vkuInitDeviceDispatchTable(*pDevice, &thisDevice.dispatch, gdpa);

    // Store device data
    {
        std::lock_guard<std::mutex> lock(global_lock);
        g_vulkanDevices[dispatch_key_from_handle(*pDevice)] = thisDevice;
    }

    // Initialize resource caches
    vkShade::Locator<vkShade::ResourceCache<vkShade::ShaderModule>>::emplace();

    spdlog::info("Layer initialization complete");
    return VK_SUCCESS;
}

VK_LAYER_EXPORT void VKAPI_CALL vkShade_DestroyDevice(VkDevice device, const VkAllocationCallbacks* pAllocator)
{
    spdlog::trace("Intercepted VkDestroyDevice");
    std::lock_guard<std::mutex> lock(global_lock);
    g_vulkanDevices.erase(dispatch_key_from_handle(device));
}
