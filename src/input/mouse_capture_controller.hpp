#pragma once

#include <chrono>

namespace vkShade
{
    enum class MouseCaptureStatus
    {
        Inactive,
        Pending,
        Active,
        Suspended,
        Unavailable,
    };

    struct MouseCaptureAttempt
    {
        MouseCaptureStatus status {MouseCaptureStatus::Inactive};
        bool retryable {false};
    };

    class MouseCaptureBackend
    {
    public:
        virtual ~MouseCaptureBackend() = default;

        virtual MouseCaptureAttempt acquire() = 0;
        virtual void release() = 0;
        virtual MouseCaptureStatus get_status() const = 0;
        virtual bool requires_pointer_constraint_permission() const { return false; }
    };

    class MouseInputInhibitor
    {
    public:
        virtual ~MouseInputInhibitor() = default;

        virtual bool inhibit() = 0;
        virtual void reconcile() = 0;
        virtual void restore() = 0;
        virtual bool is_ready_for_capture() const { return true; }
        virtual bool permits_pointer_constraint() const { return false; }
    };

    class MouseCaptureController
    {
    public:
        using Clock = std::chrono::steady_clock;
        using TimePoint = Clock::time_point;

        MouseCaptureController(
            MouseCaptureBackend& backend,
            MouseInputInhibitor& inhibitor,
            Clock::duration retryDelay);

        void set_requested(bool requested, TimePoint now);
        void update(TimePoint now);

        bool is_requested() const;
        bool is_application_inhibited() const;
        MouseCaptureStatus get_status() const;

    private:
        MouseCaptureBackend& m_backend;
        MouseInputInhibitor& m_inhibitor;
        Clock::duration m_retryDelay;
        TimePoint m_nextRetry {};
        MouseCaptureStatus m_status {MouseCaptureStatus::Inactive};
        bool m_requested {false};
        bool m_inhibitionAttempted {false};
        bool m_applicationInhibited {false};
        bool m_retryScheduled {false};
    };
} // namespace vkShade
