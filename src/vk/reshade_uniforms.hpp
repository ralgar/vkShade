#pragma once

#include <chrono>
#include <cstdint>

namespace reshadefx
{
    struct uniform;
}

namespace vkShade
{
    class VulkanBuffer;

    // Base class for a ReShade built-in uniform. These require special handling.
    class ReshadeUniform
    {
    public:
        virtual ~ReshadeUniform() = default;
        virtual void update(VulkanBuffer& buffer) = 0;

    protected:
        ReshadeUniform() = default;

        uint32_t m_size {0};
        uint32_t m_offset {0};
    };

    class FrameTimeUniform : public ReshadeUniform
    {
    public:
        FrameTimeUniform(reshadefx::uniform uniformInfo);
        void update(VulkanBuffer& buffer) override;

    private:
        std::chrono::steady_clock::time_point m_lastFrame;
    };

    class FrameCountUniform : public ReshadeUniform
    {
    public:
        FrameCountUniform(reshadefx::uniform uniformInfo);
        void update(VulkanBuffer& buffer) override;

    private:
        uint32_t m_count {0};
    };

    class DateUniform : public ReshadeUniform
    {
    public:
        DateUniform(reshadefx::uniform uniformInfo);
        void update(VulkanBuffer& buffer) override;
    };

    class TimerUniform : public ReshadeUniform
    {
    public:
        TimerUniform(reshadefx::uniform uniformInfo);
        void update(VulkanBuffer& buffer) override;

    private:
        std::chrono::steady_clock::time_point m_startTime;
    };

 class PingPongUniform : public ReshadeUniform
    {
    public:
        PingPongUniform(reshadefx::uniform uniformInfo);
        void update(VulkanBuffer& buffer) override;

    private:
        std::chrono::steady_clock::time_point m_lastFrame;

        float m_min             {0.0f};
        float m_max             {0.0f};
        float m_stepMin         {0.0f};
        float m_stepMax         {0.0f};
        float m_smoothing       {0.0f};
        float m_currentValue[2] {0.0f, 1.0f};
    };


    class RandomUniform : public ReshadeUniform
    {
    public:
        RandomUniform(reshadefx::uniform uniformInfo);
        void update(VulkanBuffer& buffer) override;

    private:
        int32_t m_max {0};
        int32_t m_min {0};
    };

    class KeyUniform : public ReshadeUniform
    {
    public:
        KeyUniform(reshadefx::uniform uniformInfo);
        void update(VulkanBuffer& buffer) override;
    };

    class MouseButtonUniform : public ReshadeUniform
    {
    public:
        MouseButtonUniform(reshadefx::uniform uniformInfo);
        void update(VulkanBuffer& buffer) override;
    };

    class MousePointUniform : public ReshadeUniform
    {
    public:
        MousePointUniform(reshadefx::uniform uniformInfo);
        void update(VulkanBuffer& buffer) override;
    };

    class MouseDeltaUniform : public ReshadeUniform
    {
    public:
        MouseDeltaUniform(reshadefx::uniform uniformInfo);
        void update(VulkanBuffer& buffer) override;
    };

    class DepthUniform : public ReshadeUniform
    {
    public:
        DepthUniform(reshadefx::uniform uniformInfo);
        void update(VulkanBuffer& buffer) override;
    };
} // namespace vkShade
