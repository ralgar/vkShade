#include "hooks.hpp"

#include <exception>

#include "core/logger.hpp"

namespace
{
    template<typename Function>
    void track_safely(Function&& function)
    {
        try
        {
            function();
        }
        catch (const std::exception& exception)
        {
            vkShade::Logger::warn("Buffer tracking failed: {}", exception.what());
        }
        catch (...)
        {
            vkShade::Logger::warn("Buffer tracking failed with an unknown error");
        }
    }

    void observe_rendering(VulkanDevice& device, const VkRenderingInfo* renderingInfo)
    {
        if (renderingInfo == nullptr)
            return;

        track_safely([&]
        {
            for (uint32_t i = 0; i < renderingInfo->colorAttachmentCount; ++i)
            {
                const VkRenderingAttachmentInfo& attachment =
                    renderingInfo->pColorAttachments[i];
                device.imageTracker->observe_view(
                    attachment.imageView,
                    vkShade::ImageObservation::ColorAttachment);
                device.imageTracker->observe_view(
                    attachment.resolveImageView,
                    vkShade::ImageObservation::ColorAttachment);
            }

            if (renderingInfo->pDepthAttachment != nullptr)
            {
                device.imageTracker->observe_view(
                    renderingInfo->pDepthAttachment->imageView,
                    vkShade::ImageObservation::DepthStencilAttachment);
                device.imageTracker->observe_view(
                    renderingInfo->pDepthAttachment->resolveImageView,
                    vkShade::ImageObservation::DepthStencilAttachment);
            }

            if (renderingInfo->pStencilAttachment != nullptr)
            {
                device.imageTracker->observe_view(
                    renderingInfo->pStencilAttachment->imageView,
                    vkShade::ImageObservation::DepthStencilAttachment);
                device.imageTracker->observe_view(
                    renderingInfo->pStencilAttachment->resolveImageView,
                    vkShade::ImageObservation::DepthStencilAttachment);
            }
        });
    }

    const VkRenderPassAttachmentBeginInfo* find_attachment_begin_info(
        const VkRenderPassBeginInfo& renderPassInfo)
    {
        auto* next = static_cast<const VkBaseInStructure*>(renderPassInfo.pNext);
        while (next != nullptr)
        {
            if (next->sType == VK_STRUCTURE_TYPE_RENDER_PASS_ATTACHMENT_BEGIN_INFO)
            {
                return reinterpret_cast<const VkRenderPassAttachmentBeginInfo*>(next);
            }
            next = next->pNext;
        }
        return nullptr;
    }

    void observe_render_pass(VulkanDevice& device,
                             const VkRenderPassBeginInfo* renderPassInfo)
    {
        if (renderPassInfo == nullptr)
            return;

        track_safely([&]
        {
            const VkRenderPassAttachmentBeginInfo* attachmentInfo =
                find_attachment_begin_info(*renderPassInfo);
            if (attachmentInfo == nullptr)
            {
                device.imageTracker->observe_framebuffer(renderPassInfo->framebuffer);
                return;
            }

            if (attachmentInfo->pAttachments == nullptr)
                return;

            for (uint32_t i = 0; i < attachmentInfo->attachmentCount; ++i)
            {
                device.imageTracker->observe_view(
                    attachmentInfo->pAttachments[i],
                    vkShade::ImageObservation::None);
            }
        });
    }
}

VK_LAYER_EXPORT VkResult VKAPI_CALL vkShade_CreateImage(
    VkDevice device, const VkImageCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator, VkImage* pImage)
{
    auto& thisDevice = get_device_from_handle(device);
    const VkResult result = thisDevice.dispatch.CreateImage(
        device, pCreateInfo, pAllocator, pImage);

    if (result == VK_SUCCESS)
    {
        track_safely([&]
        {
            thisDevice.imageTracker->register_image(
                *pImage, *pCreateInfo, vkShade::TrackedImageOrigin::Application);
        });
    }
    return result;
}

VK_LAYER_EXPORT void VKAPI_CALL vkShade_DestroyImage(
    VkDevice device, VkImage image, const VkAllocationCallbacks* pAllocator)
{
    auto& thisDevice = get_device_from_handle(device);
    track_safely([&] { thisDevice.imageTracker->unregister_image(image); });
    thisDevice.dispatch.DestroyImage(device, image, pAllocator);
}

VK_LAYER_EXPORT VkResult VKAPI_CALL vkShade_CreateImageView(
    VkDevice device, const VkImageViewCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator, VkImageView* pView)
{
    auto& thisDevice = get_device_from_handle(device);
    const VkResult result = thisDevice.dispatch.CreateImageView(
        device, pCreateInfo, pAllocator, pView);

    if (result == VK_SUCCESS)
    {
        track_safely([&]
        {
            thisDevice.imageTracker->register_view(
                *pView, pCreateInfo->image, pCreateInfo->subresourceRange);
        });
    }
    return result;
}

VK_LAYER_EXPORT void VKAPI_CALL vkShade_DestroyImageView(
    VkDevice device, VkImageView imageView, const VkAllocationCallbacks* pAllocator)
{
    auto& thisDevice = get_device_from_handle(device);
    track_safely([&] { thisDevice.imageTracker->unregister_view(imageView); });
    thisDevice.dispatch.DestroyImageView(device, imageView, pAllocator);
}

VK_LAYER_EXPORT VkResult VKAPI_CALL vkShade_CreateFramebuffer(
    VkDevice device, const VkFramebufferCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator, VkFramebuffer* pFramebuffer)
{
    auto& thisDevice = get_device_from_handle(device);
    const VkResult result = thisDevice.dispatch.CreateFramebuffer(
        device, pCreateInfo, pAllocator, pFramebuffer);

    if (result == VK_SUCCESS)
    {
        track_safely([&]
        {
            thisDevice.imageTracker->register_framebuffer(
                *pFramebuffer, pCreateInfo->attachmentCount, pCreateInfo->pAttachments);
        });
    }
    return result;
}

VK_LAYER_EXPORT void VKAPI_CALL vkShade_DestroyFramebuffer(
    VkDevice device, VkFramebuffer framebuffer,
    const VkAllocationCallbacks* pAllocator)
{
    auto& thisDevice = get_device_from_handle(device);
    track_safely([&]
    {
        thisDevice.imageTracker->unregister_framebuffer(framebuffer);
    });
    thisDevice.dispatch.DestroyFramebuffer(device, framebuffer, pAllocator);
}

VK_LAYER_EXPORT void VKAPI_CALL vkShade_CmdBeginRendering(
    VkCommandBuffer commandBuffer, const VkRenderingInfo* pRenderingInfo)
{
    auto& thisDevice = get_device_from_handle(commandBuffer);
    observe_rendering(thisDevice, pRenderingInfo);
    thisDevice.dispatch.CmdBeginRendering(commandBuffer, pRenderingInfo);
}

VK_LAYER_EXPORT void VKAPI_CALL vkShade_CmdBeginRenderingKHR(
    VkCommandBuffer commandBuffer, const VkRenderingInfo* pRenderingInfo)
{
    auto& thisDevice = get_device_from_handle(commandBuffer);
    observe_rendering(thisDevice, pRenderingInfo);
    thisDevice.dispatch.CmdBeginRenderingKHR(commandBuffer, pRenderingInfo);
}

VK_LAYER_EXPORT void VKAPI_CALL vkShade_CmdBeginRenderPass(
    VkCommandBuffer commandBuffer, const VkRenderPassBeginInfo* pRenderPassBegin,
    VkSubpassContents contents)
{
    auto& thisDevice = get_device_from_handle(commandBuffer);
    observe_render_pass(thisDevice, pRenderPassBegin);
    thisDevice.dispatch.CmdBeginRenderPass(commandBuffer, pRenderPassBegin, contents);
}

VK_LAYER_EXPORT void VKAPI_CALL vkShade_CmdBeginRenderPass2(
    VkCommandBuffer commandBuffer, const VkRenderPassBeginInfo* pRenderPassBegin,
    const VkSubpassBeginInfo* pSubpassBeginInfo)
{
    auto& thisDevice = get_device_from_handle(commandBuffer);
    observe_render_pass(thisDevice, pRenderPassBegin);
    thisDevice.dispatch.CmdBeginRenderPass2(
        commandBuffer, pRenderPassBegin, pSubpassBeginInfo);
}

VK_LAYER_EXPORT void VKAPI_CALL vkShade_CmdBeginRenderPass2KHR(
    VkCommandBuffer commandBuffer, const VkRenderPassBeginInfo* pRenderPassBegin,
    const VkSubpassBeginInfo* pSubpassBeginInfo)
{
    auto& thisDevice = get_device_from_handle(commandBuffer);
    observe_render_pass(thisDevice, pRenderPassBegin);
    thisDevice.dispatch.CmdBeginRenderPass2KHR(
        commandBuffer, pRenderPassBegin, pSubpassBeginInfo);
}
