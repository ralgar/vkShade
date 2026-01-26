#include "vulkan_hooks.hpp"

#include <stdio.h>

VK_LAYER_EXPORT VkResult VKAPI_CALL vkShade_QueuePresentKHR(VkQueue queue, const VkPresentInfoKHR *pPresentInfo)
{
    printf("Queuing present\n");

    return g_vulkanDevices[dispatch_key_from_handle(queue)].dispatchTable.QueuePresentKHR(queue, pPresentInfo);
}
