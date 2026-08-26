#include "hooks.hpp"

#include <magic_enum/magic_enum.hpp>
#include <vulkan/vk_layer.h>

#include "core/event_bus.hpp"
#include "core/logger.hpp"
#include "core/resource_cache.hpp"
#include "core/service_locator.hpp"
#include "vk/shader_module.hpp"

VK_LAYER_EXPORT VkResult VKAPI_CALL vkShade_CreateInstance(
    const VkInstanceCreateInfo*                 pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkInstance*                                 pInstance)
{
    vkShade::Logger::trace("Intercepted VkCreateInstance");

    // Step through the pNext chain until we get to the layer link info
    const VkLayerInstanceCreateInfo* layerInfo = reinterpret_cast<const VkLayerInstanceCreateInfo*>(pCreateInfo->pNext);
    while(layerInfo && (layerInfo->sType != VK_STRUCTURE_TYPE_LOADER_INSTANCE_CREATE_INFO || layerInfo->function != VK_LAYER_LINK_INFO))
    {
        layerInfo = reinterpret_cast<const VkLayerInstanceCreateInfo*>(layerInfo->pNext);
    }

    if(!layerInfo)
    {
        vkShade::Logger::error("Failed to find instance layer link info");
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    // Get the next layer's vkGetInstanceProcAddr
    // The Vulkan loader contract allows layers to modify the pNext chain during instance creation
    VkLayerInstanceCreateInfo* mutableLayerInfo = const_cast<VkLayerInstanceCreateInfo*>(layerInfo);
    PFN_vkGetInstanceProcAddr gpa = mutableLayerInfo->u.pLayerInfo->pfnNextGetInstanceProcAddr;
    mutableLayerInfo->u.pLayerInfo = mutableLayerInfo->u.pLayerInfo->pNext;  // Move chain on for next layer

    // Get vkCreateInstance from the next layer
    PFN_vkCreateInstance create_func = (PFN_vkCreateInstance)gpa(VK_NULL_HANDLE, "vkCreateInstance");
    if (!create_func)
    {
        vkShade::Logger::error("Failed to get vkCreateInstance");
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    // Get info from underlying application
	VkApplicationInfo appInfo { VK_STRUCTURE_TYPE_APPLICATION_INFO };
	if (pCreateInfo->pApplicationInfo != nullptr)
		appInfo = *pCreateInfo->pApplicationInfo;

	// vkShade requires at least Vulkan 1.3
	if (appInfo.apiVersion < VK_API_VERSION_1_3)
	{
        vkShade::Logger::info("Replacing requested Vulkan API version with 1.3");
		appInfo.apiVersion = VK_API_VERSION_1_3;
	}

    // Create modified create info with our app info
    VkInstanceCreateInfo modifiedCreateInfo = *pCreateInfo;
    modifiedCreateInfo.pApplicationInfo = &appInfo;

    // Call through to next layer
    VkResult result = create_func(&modifiedCreateInfo, pAllocator, pInstance);
    if (result != VK_SUCCESS)
    {
        vkShade::Logger::error("Failed to create instance: {}", magic_enum::enum_name(result));
        return result;
    }

    // Create our instance data struct and fill out the dispatch table
    VulkanInstance thisInstance { *pInstance, appInfo.apiVersion };
    vkuInitInstanceDispatchTable(*pInstance, &thisInstance.dispatch, gpa);

    // Store instance data
    {
        // Acquire a writer lock
        std::unique_lock lock(g_globalLock);
        g_vulkanInstances.emplace(dispatch_key_from_handle(*pInstance), thisInstance);

        // Initialize instance-level subsystems
        vkShade::Locator<vkShade::EventBus>::emplace();
    }

    return VK_SUCCESS;
}

VK_LAYER_EXPORT void VKAPI_CALL vkShade_DestroyInstance(VkInstance instance, const VkAllocationCallbacks* pAllocator)
{
    vkShade::Logger::trace("Intercepted VkDestroyInstance");

    // Lock here to prevent race conditions.
    std::unique_lock lock(g_globalLock);

    if (!instance)
        return;

    // Call down the chain to destroy the instance.
    auto& thisInstance = get_instance_from_handle(instance);
    thisInstance.dispatch.DestroyInstance(instance, pAllocator);

    // Remove from layer's bookkeeping
    g_vulkanInstances.erase(dispatch_key_from_handle(instance));
}
