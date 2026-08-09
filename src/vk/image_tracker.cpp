#include "image_tracker.hpp"

#include <algorithm>

namespace
{
    vkShade::ImageObservation operator|(vkShade::ImageObservation lhs,
                                         vkShade::ImageObservation rhs)
    {
        return static_cast<vkShade::ImageObservation>(
            static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
    }
}

void vkShade::ImageTracker::register_image(VkImage image,
                                           const VkImageCreateInfo& createInfo,
                                           TrackedImageOrigin origin)
{
    if (image == VK_NULL_HANDLE)
        return;

    std::scoped_lock lock(m_mutex);
    auto [it, inserted] = m_images.try_emplace(image);
    if (!inserted)
        return;

    it->second.snapshot = {
        .id = m_nextId++,
        .origin = origin,
        .type = createInfo.imageType,
        .extent = createInfo.extent,
        .format = createInfo.format,
        .usage = createInfo.usage,
        .observedAspects = 0,
        .observations = ImageObservation::None,
        .mipLevels = createInfo.mipLevels,
        .arrayLayers = createInfo.arrayLayers,
        .samples = createInfo.samples,
        .lastSeenFrame = m_frame,
        .useCount = 0,
    };
}

void vkShade::ImageTracker::unregister_image(VkImage image)
{
    std::scoped_lock lock(m_mutex);
    m_images.erase(image);

    std::erase_if(m_views, [image](const auto& entry)
    {
        return entry.second.image == image;
    });
}

void vkShade::ImageTracker::register_view(VkImageView view, VkImage image,
                                          const VkImageSubresourceRange& range)
{
    if (view == VK_NULL_HANDLE || image == VK_NULL_HANDLE)
        return;

    std::scoped_lock lock(m_mutex);
    m_views.insert_or_assign(view, ViewRecord {image, range});
}

void vkShade::ImageTracker::unregister_view(VkImageView view)
{
    std::scoped_lock lock(m_mutex);
    m_views.erase(view);
}

void vkShade::ImageTracker::register_framebuffer(VkFramebuffer framebuffer,
                                                 uint32_t attachmentCount,
                                                 const VkImageView* attachments)
{
    if (framebuffer == VK_NULL_HANDLE)
        return;

    std::scoped_lock lock(m_mutex);
    auto& views = m_framebuffers[framebuffer];
    views.clear();
    if (attachmentCount != 0 && attachments != nullptr)
        views.assign(attachments, attachments + attachmentCount);
}

void vkShade::ImageTracker::unregister_framebuffer(VkFramebuffer framebuffer)
{
    std::scoped_lock lock(m_mutex);
    m_framebuffers.erase(framebuffer);
}

void vkShade::ImageTracker::observe_view(VkImageView view,
                                         ImageObservation observation)
{
    std::scoped_lock lock(m_mutex);
    observe_view_locked(view, observation);
}

void vkShade::ImageTracker::observe_framebuffer(VkFramebuffer framebuffer)
{
    std::scoped_lock lock(m_mutex);
    const auto framebufferIt = m_framebuffers.find(framebuffer);
    if (framebufferIt == m_framebuffers.end())
        return;

    for (VkImageView view : framebufferIt->second)
        observe_view_locked(view, ImageObservation::None);
}

void vkShade::ImageTracker::observe_view_locked(VkImageView view,
                                                ImageObservation observation)
{
    const auto viewIt = m_views.find(view);
    if (viewIt == m_views.end())
        return;

    const auto imageIt = m_images.find(viewIt->second.image);
    if (imageIt == m_images.end())
        return;

    TrackedImageSnapshot& imageSnapshot = imageIt->second.snapshot;
    imageSnapshot.observations = imageSnapshot.observations | observation;
    imageSnapshot.observedAspects |= viewIt->second.range.aspectMask;
    imageSnapshot.lastSeenFrame = m_frame;
    ++imageSnapshot.useCount;
}

void vkShade::ImageTracker::advance_frame()
{
    std::scoped_lock lock(m_mutex);
    ++m_frame;
}

std::vector<vkShade::TrackedImageSnapshot> vkShade::ImageTracker::snapshot() const
{
    std::scoped_lock lock(m_mutex);
    std::vector<TrackedImageSnapshot> result;
    result.reserve(m_images.size());

    for (const auto& [image, record] : m_images)
    {
        const bool observed = record.snapshot.useCount != 0;
        const bool ownedByLayer = record.snapshot.origin != TrackedImageOrigin::Application;
        const bool storageCapable =
            (record.snapshot.usage & VK_IMAGE_USAGE_STORAGE_BIT) != 0;
        if (observed || ownedByLayer || storageCapable)
            result.push_back(record.snapshot);
    }

    std::ranges::sort(result, [](const auto& lhs, const auto& rhs)
    {
        if (lhs.lastSeenFrame != rhs.lastSeenFrame)
            return lhs.lastSeenFrame > rhs.lastSeenFrame;
        return lhs.id < rhs.id;
    });
    return result;
}
