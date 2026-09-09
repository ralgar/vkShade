#include "mouse_capture_controller.hpp"

namespace vkShade
{
    MouseCaptureController::MouseCaptureController(
        MouseCaptureBackend& backend,
        MouseInputInhibitor& inhibitor,
        Clock::duration retryDelay)
        : m_backend(backend)
        , m_inhibitor(inhibitor)
        , m_retryDelay(retryDelay)
    {
    }

    void MouseCaptureController::set_requested(bool requested, TimePoint now)
    {
        if (requested == m_requested)
        {
            if (requested)
                update(now);
            return;
        }

        m_requested = requested;
        if (requested)
        {
            update(now);
            return;
        }

        m_backend.release();

        if (m_applicationInhibited)
            m_inhibitor.restore();

        m_status = MouseCaptureStatus::Inactive;
        m_inhibitionAttempted = false;
        m_applicationInhibited = false;
        m_retryScheduled = false;
    }

    void MouseCaptureController::update(TimePoint now)
    {
        if (!m_requested)
            return;

        // Application inhibition is request-scoped; native acquisition may retry
        // independently without replaying a successful application transition.
        if (!m_inhibitionAttempted)
        {
            m_inhibitionAttempted = true;
            m_applicationInhibited = m_inhibitor.inhibit();
        }
        else if (m_applicationInhibited)
        {
            m_inhibitor.reconcile();
        }

        m_status = m_backend.get_status();
        if (m_status != MouseCaptureStatus::Inactive)
            return;

        if (m_retryScheduled && now < m_nextRetry)
            return;

        const MouseCaptureAttempt attempt = m_backend.acquire();
        m_status = attempt.status;
        m_retryScheduled = attempt.status == MouseCaptureStatus::Inactive && attempt.retryable;
        if (m_retryScheduled)
            m_nextRetry = now + m_retryDelay;
    }

    bool MouseCaptureController::is_requested() const
    {
        return m_requested;
    }

    bool MouseCaptureController::is_application_inhibited() const
    {
        return m_applicationInhibited;
    }

    MouseCaptureStatus MouseCaptureController::get_status() const
    {
        return m_status;
    }
} // namespace vkShade
