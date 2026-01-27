#include "vulkan_hooks.hpp"
#include "vulkan_hooks_surface.hpp"

#include <spdlog/spdlog.h>

VK_LAYER_EXPORT VkResult VKAPI_CALL vkShade_CreateWaylandSurfaceKHR(VkInstance                           instance,
                                                                    const VkWaylandSurfaceCreateInfoKHR* pCreateInfo,
                                                                    const VkAllocationCallbacks*         pAllocator,
                                                                    VkSurfaceKHR*                        pSurface)
{
    spdlog::debug("vkCreateWaylandSurfaceKHR called");

    auto& thisInstance = g_vulkanInstances[dispatch_key_from_handle(instance)];

    // Get the function pointer manually since extensions aren't in the dispatch table
    PFN_vkCreateWaylandSurfaceKHR createWaylandSurfaceKHR = (PFN_vkCreateWaylandSurfaceKHR)
        thisInstance.dispatch.GetInstanceProcAddr(instance, "vkCreateWaylandSurfaceKHR");

    VkResult result = createWaylandSurfaceKHR(instance, pCreateInfo, pAllocator, pSurface);
    if (result == VK_SUCCESS)
    {
        spdlog::debug("Wayland surface created");
    }

    return result;
}


VK_LAYER_EXPORT VkResult vkShade_CreateXcbSurfaceKHR(VkInstance                       instance,
                                                     const VkXcbSurfaceCreateInfoKHR* pCreateInfo,
                                                     const VkAllocationCallbacks*     pAllocator,
                                                     VkSurfaceKHR*                    pSurface)
{
    spdlog::debug("vkCreateXcbSurfaceKHR called");

    auto& thisInstance = g_vulkanInstances[dispatch_key_from_handle(instance)];

    // Get the function pointer manually since extensions aren't in the dispatch table
    PFN_vkCreateXcbSurfaceKHR createXcbSurfaceKHR = (PFN_vkCreateXcbSurfaceKHR)
        thisInstance.dispatch.GetInstanceProcAddr(instance, "vkCreateXcbSurfaceKHR");

    VkResult result = createXcbSurfaceKHR(instance, pCreateInfo, pAllocator, pSurface);
    if (result == VK_SUCCESS)
    {
        spdlog::debug("X11 (xcb) surface created");
    }

    return result;
}

VK_LAYER_EXPORT VkResult VKAPI_CALL vkShade_CreateXlibSurfaceKHR(VkInstance                        instance,
                                                                 const VkXlibSurfaceCreateInfoKHR* pCreateInfo,
                                                                 const VkAllocationCallbacks*      pAllocator,
                                                                 VkSurfaceKHR*                     pSurface)
{
    spdlog::debug("vkCreateXlibSurfaceKHR called");

    auto& thisInstance = g_vulkanInstances[dispatch_key_from_handle(instance)];

    // Get the function pointer manually since extensions aren't in the dispatch table
    PFN_vkCreateXlibSurfaceKHR createXlibSurfaceKHR = (PFN_vkCreateXlibSurfaceKHR)
        thisInstance.dispatch.GetInstanceProcAddr(instance, "vkCreateXlibSurfaceKHR");

    VkResult result = createXlibSurfaceKHR(instance, pCreateInfo, pAllocator, pSurface);
    if (result == VK_SUCCESS)
    {
        spdlog::debug("X11 (Xlib) surface created");
    }

    return result;
}
