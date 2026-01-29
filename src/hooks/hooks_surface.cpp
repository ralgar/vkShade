#include "input/input_manager_wayland.hpp"
#include "hooks.hpp"
#include "hooks_surface.hpp"

#include <spdlog/spdlog.h>

#include "core/service_locator.hpp"
#include "input/input_manager.hpp"

VK_LAYER_EXPORT VkResult VKAPI_CALL vkShade_CreateWaylandSurfaceKHR(VkInstance                           instance,
                                                                    const VkWaylandSurfaceCreateInfoKHR* pCreateInfo,
                                                                    const VkAllocationCallbacks*         pAllocator,
                                                                    VkSurfaceKHR*                        pSurface)
{
    spdlog::trace("Intercepted VkCreateWaylandSurfaceKHR");

    // Initialize InputManager
    vkShade::Locator<vkShade::InputManager>::emplace<vkShade::InputManagerWayland>(pCreateInfo->display);

    auto& thisInstance = get_instance_from_handle(instance);

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
    spdlog::trace("Intercepted VkCreateXcbSurfaceKHR");

    auto& thisInstance = get_instance_from_handle(instance);

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
    spdlog::trace("Intercepted VkCreateXlibSurfaceKHR");

    auto& thisInstance = get_instance_from_handle(instance);

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
