#include "hooks.hpp"
#include "hooks_surface.hpp"

#include <atomic>

#include "core/logger.hpp"

#include "core/service_locator.hpp"
#include "input/input_backend_wayland.hpp"
#include "input/input_backend_xcb.hpp"
#include "input/input_backend_xlib.hpp"
#include "input/input_manager.hpp"

namespace
{
    // The surface the current input backend was created for. The backend borrows the application's
    // window-system connection, which the application may disconnect right after destroying the surface.
    std::atomic<VkSurfaceKHR> g_inputSurface {VK_NULL_HANDLE};
}

VK_LAYER_EXPORT VkResult VKAPI_CALL vkShade_CreateWaylandSurfaceKHR(VkInstance                           instance,
                                                                    const VkWaylandSurfaceCreateInfoKHR* pCreateInfo,
                                                                    const VkAllocationCallbacks*         pAllocator,
                                                                    VkSurfaceKHR*                        pSurface)
{
    vkShade::Logger::trace("Intercepted VkCreateWaylandSurfaceKHR");

    auto& thisInstance = get_instance_from_handle(instance);

    // Get the function pointer manually since extensions aren't in the dispatch table
    PFN_vkCreateWaylandSurfaceKHR createWaylandSurfaceKHR = (PFN_vkCreateWaylandSurfaceKHR)
        thisInstance.dispatch.GetInstanceProcAddr(instance, "vkCreateWaylandSurfaceKHR");

    VkResult result = createWaylandSurfaceKHR(instance, pCreateInfo, pAllocator, pSurface);
    if (result == VK_SUCCESS)
    {
        auto* backend = vkShade::Locator<vkShade::InputManager>::has()
            ? dynamic_cast<vkShade::InputBackendWayland*>(&vkShade::Locator<vkShade::InputManager>::get())
            : nullptr;
        if (backend && backend->uses_display(pCreateInfo->display))
        {
            backend->set_surface(pCreateInfo->surface);
        }
        else
        {
            vkShade::Locator<vkShade::InputManager>::reset();
            vkShade::Locator<vkShade::InputManager>::emplace<vkShade::InputBackendWayland>(
                pCreateInfo->display, pCreateInfo->surface);
        }
        g_inputSurface = *pSurface;
        vkShade::Logger::debug("Wayland surface created");
    }

    return result;
}


VK_LAYER_EXPORT VkResult vkShade_CreateXcbSurfaceKHR(VkInstance                       instance,
                                                     const VkXcbSurfaceCreateInfoKHR* pCreateInfo,
                                                     const VkAllocationCallbacks*     pAllocator,
                                                     VkSurfaceKHR*                    pSurface)
{
    vkShade::Logger::trace("Intercepted VkCreateXcbSurfaceKHR");

    auto* xcbCreateInfo = reinterpret_cast<const VkXcbSurfaceCreateInfoKHR*>(pCreateInfo);
    xcb_connection_t* connection = xcbCreateInfo->connection;
    xcb_window_t window = xcbCreateInfo->window;

    auto& thisInstance = get_instance_from_handle(instance);

    // Get the function pointer manually since extensions aren't in the dispatch table
    PFN_vkCreateXcbSurfaceKHR createXcbSurfaceKHR = (PFN_vkCreateXcbSurfaceKHR)
        thisInstance.dispatch.GetInstanceProcAddr(instance, "vkCreateXcbSurfaceKHR");

    VkResult result = createXcbSurfaceKHR(instance, pCreateInfo, pAllocator, pSurface);
    if (result == VK_SUCCESS)
    {
        if (vkShade::Locator<vkShade::InputManager>::has())
        {
            auto* oldBackend = dynamic_cast<vkShade::InputBackendXlib*>(
                &vkShade::Locator<vkShade::InputManager>::get());
            if (oldBackend)
                oldBackend->prepare_for_surface_replacement(nullptr, 0);
        }
        vkShade::Locator<vkShade::InputManager>::reset();
        vkShade::Locator<vkShade::InputManager>::emplace<vkShade::InputBackendXcb>(connection, window);
        g_inputSurface = *pSurface;
        vkShade::Logger::debug("X11 (XCB) surface created");
    }

    return result;
}

VK_LAYER_EXPORT VkResult VKAPI_CALL vkShade_CreateXlibSurfaceKHR(VkInstance                        instance,
                                                                 const VkXlibSurfaceCreateInfoKHR* pCreateInfo,
                                                                 const VkAllocationCallbacks*      pAllocator,
                                                                 VkSurfaceKHR*                     pSurface)
{
    vkShade::Logger::trace("Intercepted VkCreateXlibSurfaceKHR");

    auto* xlibCreateInfo = reinterpret_cast<const VkXlibSurfaceCreateInfoKHR*>(pCreateInfo);
    Window window = xlibCreateInfo->window;
    Display* display = xlibCreateInfo->dpy;

    auto& thisInstance = get_instance_from_handle(instance);

    // Get the function pointer manually since extensions aren't in the dispatch table
    PFN_vkCreateXlibSurfaceKHR createXlibSurfaceKHR = (PFN_vkCreateXlibSurfaceKHR)
        thisInstance.dispatch.GetInstanceProcAddr(instance, "vkCreateXlibSurfaceKHR");

    VkResult result = createXlibSurfaceKHR(instance, pCreateInfo, pAllocator, pSurface);
    if (result == VK_SUCCESS)
    {
        if (vkShade::Locator<vkShade::InputManager>::has())
        {
            auto* oldBackend = dynamic_cast<vkShade::InputBackendXlib*>(
                &vkShade::Locator<vkShade::InputManager>::get());
            if (oldBackend)
                oldBackend->prepare_for_surface_replacement(display, window);
        }
        vkShade::Locator<vkShade::InputManager>::reset();
        vkShade::Locator<vkShade::InputManager>::emplace<vkShade::InputBackendXlib>(display, window);
        g_inputSurface = *pSurface;
        vkShade::Logger::debug("X11 (Xlib) surface created");
    }

    return result;
}

VK_LAYER_EXPORT void VKAPI_CALL vkShade_DestroySurfaceKHR(VkInstance                   instance,
                                                          VkSurfaceKHR                 surface,
                                                          const VkAllocationCallbacks* pAllocator)
{
    vkShade::Logger::trace("Intercepted VkDestroySurfaceKHR");

    // Release the input backend while the application still owns its window-system connection.
    VkSurfaceKHR expected = surface;
    if (surface != VK_NULL_HANDLE && g_inputSurface.compare_exchange_strong(expected, VK_NULL_HANDLE))
        vkShade::Locator<vkShade::InputManager>::reset();

    auto& thisInstance = get_instance_from_handle(instance);
    thisInstance.dispatch.DestroySurfaceKHR(instance, surface, pAllocator);
}
