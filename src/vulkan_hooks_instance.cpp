#include "vulkan_hooks.hpp"

#include <magic_enum/magic_enum.hpp>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <vulkan/vk_layer.h>

VK_LAYER_EXPORT VkResult VKAPI_CALL vkShade_CreateInstance(
    const VkInstanceCreateInfo*                 pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkInstance*                                 pInstance)
{
    // Initialize spdlog on first instance creation
    static bool spdlogInitialized = false;
    if (!spdlogInitialized)
    {
        auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        auto logger = std::make_shared<spdlog::logger>("vkShade", consoleSink);
        spdlog::set_default_logger(logger);
        spdlog::set_level(spdlog::level::debug);    // TODO: Provide an env var for this
        spdlogInitialized = true;
    }

    // Step through the pNext chain until we get to the layer link info
    VkLayerInstanceCreateInfo* layerInfo = (VkLayerInstanceCreateInfo*)pCreateInfo->pNext;
    while(layerInfo && (layerInfo->sType != VK_STRUCTURE_TYPE_LOADER_INSTANCE_CREATE_INFO || layerInfo->function != VK_LAYER_LINK_INFO))
    {
        layerInfo = (VkLayerInstanceCreateInfo*)layerInfo->pNext;
    }

    if(!layerInfo)
    {
        spdlog::error("Failed to find instance layer link info");
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    // Get the next layer's vkGetInstanceProcAddr
    PFN_vkGetInstanceProcAddr gpa = layerInfo->u.pLayerInfo->pfnNextGetInstanceProcAddr;
    layerInfo->u.pLayerInfo = layerInfo->u.pLayerInfo->pNext;  // Move chain on for next layer

    // Get vkCreateInstance from the next layer
    PFN_vkCreateInstance create_func = (PFN_vkCreateInstance)gpa(VK_NULL_HANDLE, "vkCreateInstance");
    if (!create_func)
    {
        spdlog::error("Failed to get vkCreateInstance");
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    // Call through to next layer
    VkResult result = create_func(pCreateInfo, pAllocator, pInstance);
    if (result != VK_SUCCESS)
    {
        spdlog::error("Failed to create instance: {}", magic_enum::enum_name(result));
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
