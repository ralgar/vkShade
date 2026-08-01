#pragma once

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <limits>
#include <string_view>

namespace vkShade
{
    inline float reshade_frame_time(std::chrono::steady_clock::duration duration)
    {
        return std::chrono::duration<float, std::milli>(duration).count();
    }

    struct ReshadeFrameState
    {
        float frameTime {0.0f};
        uint32_t frameCount {0};
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
            m_currentFrame.frameTime = reshade_frame_time(now - m_lastFrame);
            m_currentFrame.frameCount = static_cast<uint32_t>(
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

    struct ReshadePingPongState
    {
        float min {0.0f};
        float max {1.0f};
        float stepMin {0.0f};
        float stepMax {0.0f};
        float smoothing {0.0f};
        std::array<float, 2> value {0.0f, 0.0f};

        float next_step(int randomValue) const
        {
            return stepMax == 0.0f
                ? stepMin
                : stepMin + std::fmod(
                    static_cast<float>(randomValue), stepMax - stepMin + 1.0f);
        }

        void advance(float deltaTime, float step)
        {
            if (value[1] >= 0.0f)
            {
                const float smooth = std::max(0.0f, smoothing - (max - value[0]));
                value[0] += std::max(step - smooth, 0.05f) * deltaTime;
                if (value[0] >= max)
                {
                    value[0] = max;
                    value[1] = -1.0f;
                }
            }
            else
            {
                const float smooth = std::max(0.0f, smoothing - (value[0] - min));
                value[0] -= std::max(step - smooth, 0.05f) * deltaTime;
                if (value[0] <= min)
                {
                    value[0] = min;
                    value[1] = 1.0f;
                }
            }
        }
    };

    struct ReshadeRandomRange
    {
        int32_t min {0};
        int32_t max {RAND_MAX};

        int32_t value(uint32_t randomValue) const
        {
            const uint64_t width = static_cast<uint64_t>(
                std::abs(static_cast<int64_t>(max) - min)) + 1;
            return static_cast<int32_t>(
                static_cast<int64_t>(min) + static_cast<int64_t>(randomValue % width));
        }
    };

    inline bool reshade_uniform_uses_initializer(std::string_view source)
    {
        return source.empty();
    }
}
