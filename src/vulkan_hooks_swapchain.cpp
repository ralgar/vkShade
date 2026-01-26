#include "vulkan_hooks.hpp"

#include <spdlog/spdlog.h>

VK_LAYER_EXPORT VkResult VKAPI_CALL vkShade_QueuePresentKHR(VkQueue queue, const VkPresentInfoKHR *pPresentInfo)
{
    spdlog::debug("Queueing present");

    return g_vulkanDevices[dispatch_key_from_handle(queue)].dispatchTable.QueuePresentKHR(queue, pPresentInfo);
}
