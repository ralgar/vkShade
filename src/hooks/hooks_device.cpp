#include "hooks.hpp"

#include <magic_enum/magic_enum.hpp>
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include <vulkan/vk_layer.h>

#include "config/config_manager.hpp"
#include "core/event_bus.hpp"
#include "core/logger.hpp"
#include "core/service_locator.hpp"
#include "core/resource_cache.hpp"
#include "gui/gui_manager.hpp"
#include "vk/reshade_effect.hpp"
#include "vk/shader_module.hpp"

VK_LAYER_EXPORT VkResult VKAPI_CALL vkShade_CreateDevice(
    VkPhysicalDevice                            physicalDevice,
    const VkDeviceCreateInfo*                   pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkDevice*                                   pDevice)
{
    vkShade::Logger::trace("Intercepted VkCreateDevice");

    // Step through the pNext chain until we get to the layer link info
    VkLayerDeviceCreateInfo* layerInfo = (VkLayerDeviceCreateInfo*)pCreateInfo->pNext;
    while(layerInfo && (layerInfo->sType != VK_STRUCTURE_TYPE_LOADER_DEVICE_CREATE_INFO || layerInfo->function != VK_LAYER_LINK_INFO))
    {
        layerInfo = (VkLayerDeviceCreateInfo *)layerInfo->pNext;
    }

    if(!layerInfo)
    {
        vkShade::Logger::error("Failed to find device layer link info");
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
        vkShade::Logger::error("Failed to get vkCreateDevice");
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    // Check if dynamic rendering is enabled
    const VkPhysicalDeviceVulkan13Features* features13 = nullptr;
    const void* pNext = pCreateInfo->pNext;
    while (pNext)
    {
        auto* header = (VkBaseInStructure*)pNext;
        if (header->sType == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES)
        {
            features13 = (VkPhysicalDeviceVulkan13Features*)header;
            break;
        }
        pNext = header->pNext;
    }

    // If not enabled, inject it
    VkPhysicalDeviceVulkan13Features myFeatures13 = {};
    VkDeviceCreateInfo modifiedCreateInfo = *pCreateInfo;
    const VkDeviceCreateInfo* finalCreateInfo = pCreateInfo;

    if (!features13 || !features13->dynamicRendering)
    {
        vkShade::Logger::debug("Injecting Dynamic Rendering feature");

        if (features13)
        {
            myFeatures13 = *features13;
            myFeatures13.dynamicRendering = VK_TRUE;
            myFeatures13.synchronization2 = VK_TRUE;
        }
        else
        {
            myFeatures13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
            myFeatures13.pNext = const_cast<void*>(pCreateInfo->pNext);
            myFeatures13.dynamicRendering = VK_TRUE;
            myFeatures13.synchronization2 = VK_TRUE;
        }

        modifiedCreateInfo.pNext = &myFeatures13;
        finalCreateInfo = &modifiedCreateInfo;
    }

    // Call through to next layer with modified createInfo
    VkResult result = create_func(physicalDevice, finalCreateInfo, pAllocator, pDevice);
    if (result != VK_SUCCESS)
    {
        vkShade::Logger::error("Failed to create device: {}", magic_enum::enum_name(result));
        return result;
    }

    auto& thisInstance = get_instance_from_handle(physicalDevice);

    // Create device data
    VulkanDevice thisDevice;
    thisDevice.handle = *pDevice;
    thisDevice.physicalDevice = physicalDevice;
    thisDevice.instance = thisInstance.handle;

    // Initialize dispatch table
    vkuInitDeviceDispatchTable(*pDevice, &thisDevice.dispatch, gdpa);

    // Get a graphics queue
    uint32_t queueFamilyCount = 0;
	thisInstance.dispatch.GetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueProperties(queueFamilyCount);
	thisInstance.dispatch.GetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueProperties.data());

	for (uint32_t i = 0; i < pCreateInfo->queueCreateInfoCount; i++)
	{
		const auto& queueInfo = pCreateInfo->pQueueCreateInfos[i];
		assert(queueInfo.queueFamilyIndex < queueFamilyCount);

		// Find the first queue family which supports graphics and has at least one queue
		if (queueProperties[queueInfo.queueFamilyIndex].queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
            vkShade::Logger::debug("Found graphics capable queue");

            if (pCreateInfo->pQueueCreateInfos[i].pQueuePriorities[0] < 1.0f)
				vkShade::Logger::warn("Selected graphics queue has a low priority: {}", pCreateInfo->pQueueCreateInfos[i].pQueuePriorities[0]);

			thisDevice.queueFamilyIndex = queueInfo.queueFamilyIndex;
            thisDevice.dispatch.GetDeviceQueue(thisDevice.handle, thisDevice.queueFamilyIndex, 0, &thisDevice.queue);

            VkCommandPoolCreateInfo commandPoolCreateInfo;
            commandPoolCreateInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            commandPoolCreateInfo.pNext            = nullptr;
            commandPoolCreateInfo.flags            = 0;
            commandPoolCreateInfo.queueFamilyIndex = thisDevice.queueFamilyIndex;

            thisDevice.dispatch.CreateCommandPool(thisDevice.handle, &commandPoolCreateInfo, nullptr, &thisDevice.commandPool);

            break;
		}
	}

    // Initialize the Vulkan Memory Allocator
    {
        VmaVulkanFunctions vulkanFunctions = {};

        // Core functions
        vulkanFunctions.vkGetPhysicalDeviceProperties = thisInstance.dispatch.GetPhysicalDeviceProperties;
        vulkanFunctions.vkGetPhysicalDeviceMemoryProperties = thisInstance.dispatch.GetPhysicalDeviceMemoryProperties;
        vulkanFunctions.vkAllocateMemory = thisDevice.dispatch.AllocateMemory;
        vulkanFunctions.vkFreeMemory = thisDevice.dispatch.FreeMemory;
        vulkanFunctions.vkMapMemory = thisDevice.dispatch.MapMemory;
        vulkanFunctions.vkUnmapMemory = thisDevice.dispatch.UnmapMemory;
        vulkanFunctions.vkFlushMappedMemoryRanges = thisDevice.dispatch.FlushMappedMemoryRanges;
        vulkanFunctions.vkInvalidateMappedMemoryRanges = thisDevice.dispatch.InvalidateMappedMemoryRanges;
        vulkanFunctions.vkBindBufferMemory = thisDevice.dispatch.BindBufferMemory;
        vulkanFunctions.vkBindImageMemory = thisDevice.dispatch.BindImageMemory;
        vulkanFunctions.vkGetBufferMemoryRequirements = thisDevice.dispatch.GetBufferMemoryRequirements;
        vulkanFunctions.vkGetImageMemoryRequirements = thisDevice.dispatch.GetImageMemoryRequirements;
        vulkanFunctions.vkCreateBuffer = thisDevice.dispatch.CreateBuffer;
        vulkanFunctions.vkDestroyBuffer = thisDevice.dispatch.DestroyBuffer;
        vulkanFunctions.vkCreateImage = thisDevice.dispatch.CreateImage;
        vulkanFunctions.vkDestroyImage = thisDevice.dispatch.DestroyImage;
        vulkanFunctions.vkCmdCopyBuffer = thisDevice.dispatch.CmdCopyBuffer;

        // Vulkan 1.1+ (Dedicated Allocation)
        vulkanFunctions.vkGetBufferMemoryRequirements2KHR = thisDevice.dispatch.GetBufferMemoryRequirements2;
        vulkanFunctions.vkGetImageMemoryRequirements2KHR = thisDevice.dispatch.GetImageMemoryRequirements2;

        // Vulkan 1.1+ (BindMemory2)
        vulkanFunctions.vkBindBufferMemory2KHR = thisDevice.dispatch.BindBufferMemory2;
        vulkanFunctions.vkBindImageMemory2KHR = thisDevice.dispatch.BindImageMemory2;

        // Vulkan 1.1+ (Memory Budget)
        vulkanFunctions.vkGetPhysicalDeviceMemoryProperties2KHR = thisInstance.dispatch.GetPhysicalDeviceMemoryProperties2;

        // Vulkan 1.3 (Maintenance4)
        vulkanFunctions.vkGetDeviceBufferMemoryRequirements = thisDevice.dispatch.GetDeviceBufferMemoryRequirements;
        vulkanFunctions.vkGetDeviceImageMemoryRequirements = thisDevice.dispatch.GetDeviceImageMemoryRequirements;

        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.physicalDevice = thisDevice.physicalDevice;
        allocatorInfo.device = thisDevice.handle;
        allocatorInfo.instance = thisInstance.handle;
        allocatorInfo.pVulkanFunctions = &vulkanFunctions;

        VkResult vmaResult = vmaCreateAllocator(&allocatorInfo, &thisDevice.allocator);
        if (vmaResult != VK_SUCCESS)
        {
            vkShade::Logger::error("Failed to create memory allocator: {}", magic_enum::enum_name(result));
            return VK_ERROR_INITIALIZATION_FAILED;
        }
    }

    // Store device data
    {
        // Acquire a writer lock
        std::unique_lock lock(g_globalLock);
        g_vulkanDevices.emplace(dispatch_key_from_handle(*pDevice), thisDevice);
    }

    // Initialize vkShade subsystems here
    vkShade::Locator<vkShade::EventBus>::emplace();
    vkShade::Locator<vkShade::ResourceCache<vkShade::ShaderModule>>::emplace();

    vkShade::Logger::info("Layer initialization complete");
    return VK_SUCCESS;
}

VK_LAYER_EXPORT void VKAPI_CALL vkShade_DestroyDevice(VkDevice device, const VkAllocationCallbacks* pAllocator)
{
    vkShade::Logger::trace("Intercepted VkDestroyDevice");

    // Lock here to prevent race conditions.
    std::unique_lock lock(g_globalLock);

    if (!device)
        return;

    auto& thisDevice = get_device_from_handle(device);

    // First destroy anything that calls into Vulkan
    if (vkShade::Locator<vkShade::GuiManager>::has())
        vkShade::Locator<vkShade::GuiManager>::reset();

    thisDevice.dispatch.DestroyCommandPool(thisDevice.handle, thisDevice.commandPool, nullptr);

    vmaDestroyAllocator(thisDevice.allocator);

    // Call down the chain to complete device destruction.
    thisDevice.dispatch.DestroyDevice(device, pAllocator);

    // Remove the VulkanDevice from layer's bookkeeping
    g_vulkanDevices.erase(dispatch_key_from_handle(device));
}

VK_LAYER_EXPORT VkResult VKAPI_CALL vkShade_AllocateCommandBuffers(VkDevice                           device,
                                                                   const VkCommandBufferAllocateInfo* pAllocateInfo,
                                                                   VkCommandBuffer*                   pCommandBuffers)
{
    auto& thisDevice = get_device_from_handle(device);
    VkResult result = thisDevice.dispatch.AllocateCommandBuffers(device, pAllocateInfo, pCommandBuffers);

    // Command buffers are dispatchable objects — the loader normally fixes up the dispatch table
    // pointer in the handle, but only when the call goes through the loader trampoline. When a
    // layer allocates command buffers internally via its own dispatch table, the trampoline is
    // bypassed and the fixup never happens. We do it manually here so the validation layer can
    // look up the device from any command buffer handle, regardless of who allocated it.
    // NOTE: This is required for validation layers to work.
    if (result == VK_SUCCESS)
    {
        for (uint32_t i = 0; i < pAllocateInfo->commandBufferCount; i++)
        {
            *reinterpret_cast<void**>(pCommandBuffers[i]) = *reinterpret_cast<void**>(device);
        }
    }

    return result;
}
