#pragma once

#include <chrono>
#include <cstdint>
#include <limits>

namespace vkShade
{
    inline float reshade_frame_time(std::chrono::steady_clock::duration duration)
    {
        return std::chrono::duration<float, std::milli>(duration).count();
    }

    struct ReshadeFrameState
    {
        float frame_time {0.0f};
        uint32_t frame_count {0};
        float timer {0.0f};
    };

    class ReshadeRuntime
    {
    public:
        using Clock = std::chrono::steady_clock;

        explicit ReshadeRuntime(Clock::time_point start = Clock::now())
            : m_start(start), m_lastFrame(start)
        {
        }

        const ReshadeFrameState& begin_frame(Clock::time_point now = Clock::now())
        {
            m_currentFrame.frame_time = reshade_frame_time(now - m_lastFrame);
            m_currentFrame.frame_count = static_cast<uint32_t>(
                m_frameCount % std::numeric_limits<uint32_t>::max());
            m_currentFrame.timer = reshade_frame_time(now - m_start);

            m_lastFrame = now;
            ++m_frameCount;
            return m_currentFrame;
        }

        const ReshadeFrameState& current_frame() const { return m_currentFrame; }

    private:
        Clock::time_point m_start;
        Clock::time_point m_lastFrame;
        uint64_t m_frameCount {0};
        ReshadeFrameState m_currentFrame;
    };
}
