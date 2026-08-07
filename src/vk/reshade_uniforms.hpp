#pragma once

#include <chrono>
#include <cstdint>

#include "vk/reshade_runtime.hpp"

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
        ReshadePingPongState m_state;
    };


    class RandomUniform : public ReshadeUniform
    {
    public:
        RandomUniform(reshadefx::uniform uniform);
        void update(VulkanBuffer& buffer, const ReshadeFrameState& frame) override;

    private:
        ReshadeRandomRange m_range;
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
