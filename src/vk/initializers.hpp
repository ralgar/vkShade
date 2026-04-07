#pragma once

#include <span>

#include <vulkan/vulkan.h>

namespace vkinit
{
    VkCommandBufferAllocateInfo command_buffer_allocate_info(VkCommandPool pool, uint32_t count = 1);
    VkCommandBufferBeginInfo    command_buffer_begin_info(VkCommandBufferUsageFlags flags = 0);
    VkCommandBufferSubmitInfo   command_buffer_submit_info(VkCommandBuffer cmd);
    VkCommandPoolCreateInfo     command_pool_create_info(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0);
    VkRenderingAttachmentInfo   depth_attachment_info(VkImageView   view,
                                                      VkImageLayout layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    VkImageCreateInfo           image_create_info(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent);
    VkImageSubresourceRange     image_subresource_range(VkImageAspectFlags aspectMask);
    VkImageViewCreateInfo       imageview_create_info(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags);
    VkRenderingAttachmentInfo   rendering_attachment_info(VkImageView         view,
                                                          const VkClearValue* clear,
                                                          VkImageLayout       layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    VkRenderingInfo             rendering_info(VkExtent2D                           renderExtent,
                                               std::span<VkRenderingAttachmentInfo> colorAttachments,
                                               const VkRenderingAttachmentInfo*     depthAttachment = nullptr,
                                               const VkRenderingAttachmentInfo*     stencilAttachment = nullptr);
    VkSemaphoreSubmitInfo       semaphore_submit_info(VkPipelineStageFlags2 stageMask, VkSemaphore semaphore);
    VkSubmitInfo2               submit_info(const VkCommandBufferSubmitInfo* cmd,
                                            const VkSemaphoreSubmitInfo*     signalSemaphoreInfo,
                                            const VkSemaphoreSubmitInfo*     waitSemaphoreInfo);
} // namespace vkinit
