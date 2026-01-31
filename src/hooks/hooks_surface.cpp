#include "hooks.hpp"
#include "hooks_surface.hpp"

#include <spdlog/spdlog.h>

#include "core/service_locator.hpp"
#include "input/input_backend_wayland.hpp"
#include "input/input_backend_xcb.hpp"
#include "input/input_backend_xlib.hpp"
#include "input/input_manager.hpp"

VK_LAYER_EXPORT VkResult VKAPI_CALL vkShade_CreateWaylandSurfaceKHR(VkInstance                           instance,
                                                                    const VkWaylandSurfaceCreateInfoKHR* pCreateInfo,
                                                                    const VkAllocationCallbacks*         pAllocator,
                                                                    VkSurfaceKHR*                        pSurface)
{
    spdlog::trace("Intercepted VkCreateWaylandSurfaceKHR");

    // Initialize InputManager
    vkShade::Locator<vkShade::InputManager>::emplace<vkShade::InputBackendWayland>(pCreateInfo->display);

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

    // Initialize InputManager
    auto* xcbCreateInfo = reinterpret_cast<const VkXcbSurfaceCreateInfoKHR*>(pCreateInfo);
    xcb_connection_t* connection = xcbCreateInfo->connection;
    xcb_window_t window = xcbCreateInfo->window;
    vkShade::Locator<vkShade::InputManager>::emplace<vkShade::InputBackendXcb>(connection, window);

    auto& thisInstance = get_instance_from_handle(instance);

    // Get the function pointer manually since extensions aren't in the dispatch table
    PFN_vkCreateXcbSurfaceKHR createXcbSurfaceKHR = (PFN_vkCreateXcbSurfaceKHR)
        thisInstance.dispatch.GetInstanceProcAddr(instance, "vkCreateXcbSurfaceKHR");

    VkResult result = createXcbSurfaceKHR(instance, pCreateInfo, pAllocator, pSurface);
    if (result == VK_SUCCESS)
    {
        spdlog::debug("X11 (XCB) surface created");
        spdlog::warn("XCB input is currently unsupported");
    }

    return result;
}

VK_LAYER_EXPORT VkResult VKAPI_CALL vkShade_CreateXlibSurfaceKHR(VkInstance                        instance,
                                                                 const VkXlibSurfaceCreateInfoKHR* pCreateInfo,
                                                                 const VkAllocationCallbacks*      pAllocator,
                                                                 VkSurfaceKHR*                     pSurface)
{
    spdlog::trace("Intercepted VkCreateXlibSurfaceKHR");

    // Initialize InputManager
    auto* xlibCreateInfo = reinterpret_cast<const VkXlibSurfaceCreateInfoKHR*>(pCreateInfo);
    Window window = xlibCreateInfo->window;
    Display* display = xlibCreateInfo->dpy;
    vkShade::Locator<vkShade::InputManager>::emplace<vkShade::InputBackendXlib>(display, window);

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
