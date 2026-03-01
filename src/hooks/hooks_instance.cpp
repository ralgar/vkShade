#include "hooks.hpp"

#include <cstdlib>

#include <magic_enum/magic_enum.hpp>
#include <spdlog/sinks/basic_file_sink.h>
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
        std::vector<spdlog::sink_ptr> sinks;

        // Always add console sink
        auto consoleSink = std::make_shared<spdlog::sinks::stderr_color_sink_mt>();
        sinks.push_back(consoleSink);

        // Optionally add file sink
        const char* logFileEnv = std::getenv("VKSHADE_LOG_FILE");
        if (logFileEnv != nullptr)
        {
            auto fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFileEnv, true);
            sinks.push_back(fileSink);
        }

        auto logger = std::make_shared<spdlog::logger>("vkShade", sinks.begin(), sinks.end());
        spdlog::set_default_logger(logger);

        // Set log level
        const char* logLevelEnv = std::getenv("VKSHADE_LOG_LEVEL");
        std::string level = logLevelEnv ? logLevelEnv : "";

        if (level == "trace")
            spdlog::set_level(spdlog::level::trace);
        else if (level == "debug")
            spdlog::set_level(spdlog::level::debug);
        else if (level == "info")
            spdlog::set_level(spdlog::level::info);
        else if (level == "warn" || level == "warning")
            spdlog::set_level(spdlog::level::warn);
        else if (level == "error")
            spdlog::set_level(spdlog::level::err);
        else if (level == "critical")
            spdlog::set_level(spdlog::level::critical);
        else if (level == "off")
            spdlog::set_level(spdlog::level::off);
        else
            spdlog::set_level(spdlog::level::info);

        spdlogInitialized = true;
    }

    spdlog::trace("Intercepted VkCreateInstance");

    // Step through the pNext chain until we get to the layer link info
    const VkLayerInstanceCreateInfo* layerInfo = reinterpret_cast<const VkLayerInstanceCreateInfo*>(pCreateInfo->pNext);
    while(layerInfo && (layerInfo->sType != VK_STRUCTURE_TYPE_LOADER_INSTANCE_CREATE_INFO || layerInfo->function != VK_LAYER_LINK_INFO))
    {
        layerInfo = reinterpret_cast<const VkLayerInstanceCreateInfo*>(layerInfo->pNext);
    }

    if(!layerInfo)
    {
        spdlog::error("Failed to find instance layer link info");
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
        spdlog::error("Failed to get vkCreateInstance");
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    // Get info from underlying application
	VkApplicationInfo appInfo { VK_STRUCTURE_TYPE_APPLICATION_INFO };
	if (pCreateInfo->pApplicationInfo != nullptr)
		appInfo = *pCreateInfo->pApplicationInfo;

	// vkShade requires at least Vulkan 1.3
	if (appInfo.apiVersion < VK_API_VERSION_1_3)
	{
        spdlog::info("Replacing requested Vulkan API version with 1.3");
		appInfo.apiVersion = VK_API_VERSION_1_3;
	}

    // Create modified create info with our app info
    VkInstanceCreateInfo modifiedCreateInfo = *pCreateInfo;
    modifiedCreateInfo.pApplicationInfo = &appInfo;

    // Call through to next layer
    VkResult result = create_func(&modifiedCreateInfo, pAllocator, pInstance);
    if (result != VK_SUCCESS)
    {
        spdlog::error("Failed to create instance: {}", magic_enum::enum_name(result));
        return result;
    }

    // Create our instance data struct and fill out the dispatch table
    VulkanInstance thisInstance { *pInstance, appInfo.apiVersion };
    vkuInitInstanceDispatchTable(*pInstance, &thisInstance.dispatch, gpa);

    // Store instance data
    {
        std::lock_guard<std::mutex> lock(global_lock);
        g_vulkanInstances.emplace(dispatch_key_from_handle(*pInstance), thisInstance);
    }

    return VK_SUCCESS;
}

VK_LAYER_EXPORT void VKAPI_CALL vkShade_DestroyInstance(VkInstance instance, const VkAllocationCallbacks* pAllocator)
{
    spdlog::trace("Intercepted VkDestroyInstance");

    // Lock here to prevent race conditions.
    std::lock_guard<std::mutex> lock(global_lock);

    if (!instance)
        return;

    // Call down the chain to destroy the instance.
    auto& thisInstance = get_instance_from_handle(instance);
    thisInstance.dispatch.DestroyInstance(instance, pAllocator);

    // Remove from layer's bookkeeping
    g_vulkanInstances.erase(dispatch_key_from_handle(instance));
}
