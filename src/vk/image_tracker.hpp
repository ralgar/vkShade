#pragma once

#include <cstdint>
#include <mutex>
#include <unordered_map>
#include <vector>

#include <vulkan/vulkan_core.h>

namespace vkShade
{
    enum class TrackedImageOrigin : uint8_t
    {
        Application,
        Swapchain,
        VkShade,
    };

    enum class ImageObservation : uint8_t
    {
        None = 0,
        ColorAttachment = 1 << 0,
        DepthStencilAttachment = 1 << 1,
    };

    struct TrackedImageSnapshot
    {
        uint64_t id;
        TrackedImageOrigin origin;
        VkImageType type;
        VkExtent3D extent;
        VkFormat format;
        VkImageUsageFlags usage;
        VkImageAspectFlags observedAspects;
        ImageObservation observations;
        uint32_t mipLevels;
        uint32_t arrayLayers;
        VkSampleCountFlagBits samples;
        uint64_t lastSeenFrame;
        uint64_t useCount;
    };

    class ImageTracker
    {
    public:
        void register_image(VkImage image, const VkImageCreateInfo& createInfo,
                            TrackedImageOrigin origin);
        void unregister_image(VkImage image);

        void register_view(VkImageView view, VkImage image,
                           const VkImageSubresourceRange& range);
        void unregister_view(VkImageView view);

        void register_framebuffer(VkFramebuffer framebuffer,
                                  uint32_t attachmentCount,
                                  const VkImageView* attachments);
        void unregister_framebuffer(VkFramebuffer framebuffer);

        void observe_view(VkImageView view, ImageObservation observation);
        void observe_framebuffer(VkFramebuffer framebuffer);
        void advance_frame();

        std::vector<TrackedImageSnapshot> snapshot() const;

    private:
        struct ImageRecord
        {
            TrackedImageSnapshot snapshot;
        };

        struct ViewRecord
        {
            VkImage image;
            VkImageSubresourceRange range;
        };

        void observe_view_locked(VkImageView view, ImageObservation observation);

        mutable std::mutex m_mutex;
        std::unordered_map<VkImage, ImageRecord> m_images;
        std::unordered_map<VkImageView, ViewRecord> m_views;
        std::unordered_map<VkFramebuffer, std::vector<VkImageView>> m_framebuffers;
        uint64_t m_nextId = 1;
        uint64_t m_frame = 0;
    };
} // namespace vkShade
