#pragma once

#include <cstdint>
#include <memory>

#include "core/diagnostics_state.hpp"
#include "core/rolling_statistics.hpp"

namespace vkShade
{
    class DiagnosticsOverlay
    {
    public:
        explicit DiagnosticsOverlay(std::shared_ptr<DiagnosticsState> diagnosticsState);

        void render();

    private:
        static constexpr std::size_t TimingWindowSize = 120;

        void render_performance();
        void reset_performance_samples();

        std::shared_ptr<DiagnosticsState> m_diagnosticsState;
        RollingStatistics<TimingWindowSize> m_totalGpuStatistics;
        RollingStatistics<TimingWindowSize> m_effectsGpuStatistics;
        RollingStatistics<TimingWindowSize> m_otherGpuStatistics;
        uint32_t m_lastGpuTimingSample {0};
    };
} // namespace vkShade
