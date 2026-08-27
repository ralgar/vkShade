#pragma once

#include <memory>

#include <vulkan/vulkan_core.h>

#include "core/diagnostics_state.hpp"
#include "gpu_timestamp.hpp"
#include "object.hpp"

namespace vkShade
{
    class GpuTiming : public VulkanObject
    {
    public:
        GpuTiming(VulkanDevice& device, std::shared_ptr<DiagnosticsState> state);
        ~GpuTiming() override;

        void collect_results();
        bool begin_frame(VkCommandBuffer commandBuffer);
        void begin_effects(VkCommandBuffer commandBuffer);
        void end_effects(VkCommandBuffer commandBuffer);
        void end_frame(VkCommandBuffer commandBuffer);

    private:
        enum Query : uint32_t
        {
            TotalBegin,
            EffectsBegin,
            EffectsEnd,
            TotalEnd,
            QueryCount,
        };

        bool ensure_query_pool();
        double milliseconds(uint64_t begin, uint64_t end) const;

        std::shared_ptr<DiagnosticsState> m_state;
        VkQueryPool m_queryPool {VK_NULL_HANDLE};
        bool m_creationAttempted {false};
        bool m_pendingResults {false};
        bool m_recording {false};
    };
} // namespace vkShade
