#pragma once

#include <atomic>

namespace vkShade
{
    struct DiagnosticsState
    {
        std::atomic_bool showPerformanceOverlay {false};

        std::atomic_bool gpuTimingSupported {false};
        std::atomic_bool gpuTimingValid {false};
        std::atomic<double> totalGpuMilliseconds {0.0};
        std::atomic<double> effectsGpuMilliseconds {0.0};
    };
} // namespace vkShade
