#pragma once

#include <chrono>
#include <cstdint>

namespace reshadefx
{
    struct uniform;
}

namespace vkShade
{
    struct ReshadeFrameState;
    class VulkanBuffer;

    // Base class for a ReShade built-in uniform. These require special handling.
    class ReshadeUniform
    {
    public:
        virtual ~ReshadeUniform() = default;
        virtual void update(VulkanBuffer& buffer, const ReshadeFrameState& frame) = 0;

    protected:
        ReshadeUniform() = default;

        uint32_t m_size {0};
        uint32_t m_offset {0};
    };

    class FrameTimeUniform : public ReshadeUniform
    {
    public:
        FrameTimeUniform(reshadefx::uniform uniform);
        void update(VulkanBuffer& buffer, const ReshadeFrameState& frame) override;
    };

    class FrameCountUniform : public ReshadeUniform
    {
    public:
        FrameCountUniform(reshadefx::uniform uniform);
        void update(VulkanBuffer& buffer, const ReshadeFrameState& frame) override;
    };

    class DateUniform : public ReshadeUniform
    {
    public:
        DateUniform(reshadefx::uniform uniform);
        void update(VulkanBuffer& buffer, const ReshadeFrameState& frame) override;
    };

    class TimerUniform : public ReshadeUniform
    {
    public:
        TimerUniform(reshadefx::uniform uniform);
        void update(VulkanBuffer& buffer, const ReshadeFrameState& frame) override;
    };

 class PingPongUniform : public ReshadeUniform
    {
    public:
        PingPongUniform(reshadefx::uniform uniform);
        void update(VulkanBuffer& buffer, const ReshadeFrameState& frame) override;

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
        RandomUniform(reshadefx::uniform uniform);
        void update(VulkanBuffer& buffer, const ReshadeFrameState& frame) override;

    private:
        int32_t m_max {0};
        int32_t m_min {0};
    };

    class KeyUniform : public ReshadeUniform
    {
    public:
        KeyUniform(reshadefx::uniform uniform);
        void update(VulkanBuffer& buffer, const ReshadeFrameState& frame) override;
    };

    class MouseButtonUniform : public ReshadeUniform
    {
    public:
        MouseButtonUniform(reshadefx::uniform uniform);
        void update(VulkanBuffer& buffer, const ReshadeFrameState& frame) override;
    };

    class MousePointUniform : public ReshadeUniform
    {
    public:
        MousePointUniform(reshadefx::uniform uniform);
        void update(VulkanBuffer& buffer, const ReshadeFrameState& frame) override;
    };

    class MouseDeltaUniform : public ReshadeUniform
    {
    public:
        MouseDeltaUniform(reshadefx::uniform uniform);
        void update(VulkanBuffer& buffer, const ReshadeFrameState& frame) override;
    };

    class MouseWheelUniform : public ReshadeUniform
    {
    public:
        MouseWheelUniform(reshadefx::uniform uniform);
        void update(VulkanBuffer& buffer, const ReshadeFrameState& frame) override;
    };

    class DepthUniform : public ReshadeUniform
    {
    public:
        DepthUniform(reshadefx::uniform uniform);
        void update(VulkanBuffer& buffer, const ReshadeFrameState& frame) override;
    };

    class OverlayOpenUniform : public ReshadeUniform
    {
    public:
        OverlayOpenUniform(reshadefx::uniform uniform);
        void update(VulkanBuffer& buffer, const ReshadeFrameState& frame) override;
    };

    class OverlayActiveUniform : public ReshadeUniform
    {
    public:
        OverlayActiveUniform(reshadefx::uniform uniform);
        void update(VulkanBuffer& buffer, const ReshadeFrameState& frame) override;
    };

    class OverlayHoveredUniform : public ReshadeUniform
    {
    public:
        OverlayHoveredUniform(reshadefx::uniform uniform);
        void update(VulkanBuffer& buffer, const ReshadeFrameState& frame) override;
    };

    class ScreenshotUniform : public ReshadeUniform
    {
    public:
        ScreenshotUniform(reshadefx::uniform uniform);
        void update(VulkanBuffer& buffer, const ReshadeFrameState& frame) override;
    };
} // namespace vkShade
