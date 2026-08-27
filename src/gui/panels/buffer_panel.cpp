#include "buffer_panel.hpp"

#include <algorithm>
#include <array>
#include <iterator>
#include <string>
#include <utility>

#include <imgui.h>
#include <magic_enum/magic_enum.hpp>

#include "core/logger.hpp"

namespace
{
    const char* image_origin_name(vkShade::TrackedImageOrigin origin)
    {
        switch (origin)
        {
            case vkShade::TrackedImageOrigin::Application: return "Application";
            case vkShade::TrackedImageOrigin::Swapchain: return "Swapchain";
            case vkShade::TrackedImageOrigin::VkShade: return "vkShade";
        }
        return "Unknown";
    }

    bool has_observation(vkShade::ImageObservation observations,
                         vkShade::ImageObservation test)
    {
        return (static_cast<uint8_t>(observations) & static_cast<uint8_t>(test)) != 0;
    }

    const char* image_role_name(const vkShade::TrackedImageSnapshot& image)
    {
        if (image.origin == vkShade::TrackedImageOrigin::Swapchain)
            return "Swapchain";
        if (has_observation(image.observations,
                            vkShade::ImageObservation::DepthStencilAttachment))
            return "Depth/stencil attachment";
        if (has_observation(image.observations,
                            vkShade::ImageObservation::ColorAttachment))
            return "Color attachment";
        if (image.observedAspects & (VK_IMAGE_ASPECT_DEPTH_BIT |
                                     VK_IMAGE_ASPECT_STENCIL_BIT))
            return "Depth/stencil attachment";
        if (image.observedAspects & VK_IMAGE_ASPECT_COLOR_BIT)
            return "Color attachment";
        if (image.usage & VK_IMAGE_USAGE_STORAGE_BIT)
            return "Storage capable";
        if (image.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
            return "Depth/stencil capable";
        if (image.usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
            return "Color capable";
        if (image.usage & VK_IMAGE_USAGE_SAMPLED_BIT)
            return "Sampled image";
        return "Image";
    }

    std::string image_usage_string(VkImageUsageFlags usage)
    {
        struct UsageName
        {
            VkImageUsageFlagBits flag;
            const char* name;
        };
        constexpr std::array usageNames = {
            UsageName {VK_IMAGE_USAGE_TRANSFER_SRC_BIT, "Transfer src"},
            UsageName {VK_IMAGE_USAGE_TRANSFER_DST_BIT, "Transfer dst"},
            UsageName {VK_IMAGE_USAGE_SAMPLED_BIT, "Sampled"},
            UsageName {VK_IMAGE_USAGE_STORAGE_BIT, "Storage"},
            UsageName {VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, "Color attachment"},
            UsageName {VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, "Depth/stencil"},
            UsageName {VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT, "Transient"},
            UsageName {VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, "Input attachment"},
        };

        std::string result;
        for (const auto& entry : usageNames)
        {
            if (!(usage & entry.flag))
                continue;
            if (!result.empty())
                result.append(", ");
            result.append(entry.name);
        }
        return result.empty() ? "None" : result;
    }
}

vkShade::BufferPanel::BufferPanel(std::shared_ptr<ImageTracker> imageTracker)
    : m_imageTracker(std::move(imageTracker))
{}

void vkShade::BufferPanel::render()
{
    if (m_trackedImages.empty() || ImGui::GetTime() >= m_nextRefresh)
    {
        try
        {
            m_trackedImages = m_imageTracker->snapshot();
        }
        catch (const std::exception& exception)
        {
            Logger::warn("Unable to refresh buffer diagnostics: {}", exception.what());
        }
        m_nextRefresh = ImGui::GetTime() + 0.25;
    }

    ImGui::TextUnformatted("Prioritize:");
    ImGui::SameLine();
    ImGui::Checkbox("Color", &m_prioritizeColorBuffers);
    ImGui::SameLine();
    ImGui::Checkbox("Depth/Stencil", &m_prioritizeDepthBuffers);
    ImGui::SameLine();
    ImGui::Checkbox("Storage", &m_prioritizeStorageBuffers);
    ImGui::SameLine();
    ImGui::Checkbox("Swapchain", &m_prioritizeSwapchainBuffers);
    ImGui::SameLine();
    ImGui::Checkbox("Internal", &m_prioritizeInternalBuffers);

    const bool hasPriority = m_prioritizeColorBuffers ||
                             m_prioritizeDepthBuffers ||
                             m_prioritizeStorageBuffers ||
                             m_prioritizeSwapchainBuffers ||
                             m_prioritizeInternalBuffers;

    std::vector<const TrackedImageSnapshot*> displayedImages;
    displayedImages.reserve(m_trackedImages.size());
    std::transform(
        m_trackedImages.begin(), m_trackedImages.end(),
        std::back_inserter(displayedImages),
        [](const TrackedImageSnapshot& image) { return &image; });

    size_t prioritizedCount = 0;
    if (hasPriority)
    {
        const auto priorityEnd = std::stable_partition(
            displayedImages.begin(), displayedImages.end(),
            [this](const TrackedImageSnapshot* image)
            {
                return (m_prioritizeColorBuffers &&
                        (image->usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)) ||
                       (m_prioritizeDepthBuffers &&
                        (image->usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)) ||
                       (m_prioritizeStorageBuffers &&
                        (image->usage & VK_IMAGE_USAGE_STORAGE_BIT)) ||
                       (m_prioritizeSwapchainBuffers &&
                        image->origin == TrackedImageOrigin::Swapchain) ||
                       (m_prioritizeInternalBuffers &&
                        image->origin == TrackedImageOrigin::VkShade);
            });
        prioritizedCount = std::distance(displayedImages.begin(), priorityEnd);

        ImGui::TextDisabled(
            "%zu of %zu tracked images prioritized; none are hidden.",
            prioritizedCount, m_trackedImages.size());
    }
    else
    {
        ImGui::TextDisabled(
            "%zu tracked images. Select categories to prioritize them without hiding others.",
            m_trackedImages.size());
    }
    ImGui::Separator();

    constexpr ImGuiTableFlags tableFlags =
        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
        ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable |
        ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY |
        ImGuiTableFlags_SizingFixedFit;

    if (!ImGui::BeginTable("TrackedBuffers", 10, tableFlags, ImVec2(0, 0)))
        return;

    ImGui::TableSetupScrollFreeze(2, 1);
    ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed);
    ImGui::TableSetupColumn("Role", ImGuiTableColumnFlags_WidthFixed, 170.0f);
    ImGui::TableSetupColumn("Origin", ImGuiTableColumnFlags_WidthFixed);
    ImGui::TableSetupColumn("Extent", ImGuiTableColumnFlags_WidthFixed);
    ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed);
    ImGui::TableSetupColumn("Format", ImGuiTableColumnFlags_WidthFixed, 190.0f);
    ImGui::TableSetupColumn("Samples", ImGuiTableColumnFlags_WidthFixed);
    ImGui::TableSetupColumn("Mips / Layers", ImGuiTableColumnFlags_WidthFixed);
    ImGui::TableSetupColumn("Usage", ImGuiTableColumnFlags_WidthFixed, 360.0f);
    ImGui::TableSetupColumn("Last frame / Observations", ImGuiTableColumnFlags_WidthFixed);
    ImGui::TableHeadersRow();

    ImGuiListClipper clipper;
    clipper.Begin(static_cast<int>(displayedImages.size()));
    while (clipper.Step())
    {
        for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row)
        {
            const TrackedImageSnapshot& image = *displayedImages[row];
            const bool dimRow = hasPriority &&
                                static_cast<size_t>(row) >= prioritizedCount;
            if (dimRow)
            {
                ImGui::PushStyleColor(
                    ImGuiCol_Text,
                    ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
            }
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%llu", static_cast<unsigned long long>(image.id));
            ImGui::TableSetColumnIndex(1);
            ImGui::TextUnformatted(image_role_name(image));
            ImGui::TableSetColumnIndex(2);
            ImGui::TextUnformatted(image_origin_name(image.origin));
            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%u x %u x %u", image.extent.width,
                        image.extent.height, image.extent.depth);
            ImGui::TableSetColumnIndex(4);
            const auto typeName = magic_enum::enum_name(image.type);
            ImGui::TextUnformatted(typeName.data(), typeName.data() + typeName.size());
            ImGui::TableSetColumnIndex(5);
            const auto formatName = magic_enum::enum_name(image.format);
            ImGui::TextUnformatted(formatName.data(), formatName.data() + formatName.size());
            ImGui::TableSetColumnIndex(6);
            ImGui::Text("%u", static_cast<uint32_t>(image.samples));
            ImGui::TableSetColumnIndex(7);
            ImGui::Text("%u / %u", image.mipLevels, image.arrayLayers);
            ImGui::TableSetColumnIndex(8);
            const std::string usage = image_usage_string(image.usage);
            ImGui::TextUnformatted(usage.c_str());
            ImGui::TableSetColumnIndex(9);
            if (image.useCount == 0)
                ImGui::TextUnformatted("- / -");
            else
                ImGui::Text("%llu / %llu",
                            static_cast<unsigned long long>(image.lastSeenFrame),
                            static_cast<unsigned long long>(image.useCount));

            if (dimRow)
                ImGui::PopStyleColor();
        }
    }

    ImGui::EndTable();
}
