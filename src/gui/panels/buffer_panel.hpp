#pragma once

#include "panel.hpp"

#include <memory>
#include <vector>

#include "vk/image_tracker.hpp"

namespace vkShade
{
    class BufferPanel : public GuiPanel
    {
    public:
        explicit BufferPanel(std::shared_ptr<ImageTracker> imageTracker);

        void render() override;

    private:
        std::shared_ptr<ImageTracker> m_imageTracker;
        std::vector<TrackedImageSnapshot> m_trackedImages;
        double m_nextRefresh = 0.0;
        bool m_prioritizeColorBuffers = false;
        bool m_prioritizeDepthBuffers = false;
        bool m_prioritizeStorageBuffers = false;
        bool m_prioritizeSwapchainBuffers = false;
        bool m_prioritizeInternalBuffers = false;
    };
} // namespace vkShade
