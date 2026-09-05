#include "virtual_mouse_cursor.hpp"

#include <algorithm>
#include <cmath>

namespace
{
    constexpr float WARP_TOLERANCE = 1.0f;
    constexpr float DISCONTINUITY_FRACTION = 0.25f;

    bool are_positions_near(glm::vec2 first, glm::vec2 second, float tolerance)
    {
        return std::abs(first.x - second.x) <= tolerance &&
               std::abs(first.y - second.y) <= tolerance;
    }
}

namespace vkShade
{
    VirtualMouseCursor::VirtualMouseCursor(glm::vec2 bounds)
    {
        set_bounds(bounds);
    }

    void VirtualMouseCursor::reset(glm::vec2 position)
    {
        m_position = clamp_to_bounds(position);
        m_lastRootPosition.reset();
    }

    void VirtualMouseCursor::reset(glm::vec2 position, glm::vec2 rootPosition)
    {
        m_position = clamp_to_bounds(position);
        m_lastRootPosition = rootPosition;
    }

    void VirtualMouseCursor::set_bounds(glm::vec2 bounds)
    {
        m_bounds = {std::max(bounds.x, 0.0f), std::max(bounds.y, 0.0f)};
        m_position = clamp_to_bounds(m_position);
    }

    void VirtualMouseCursor::set_warp_position(glm::vec2 rootPosition)
    {
        m_warpPosition = rootPosition;
    }

    void VirtualMouseCursor::clear_warp_position()
    {
        m_warpPosition.reset();
    }

    std::optional<glm::vec2> VirtualMouseCursor::observe_root_motion(
        glm::vec2 rootPosition,
        std::optional<glm::vec2> discontinuityPosition)
    {
        // Relative-mode applications recenter the OS pointer. Treat that event
        // as a new motion baseline rather than as user movement.
        if (m_warpPosition && are_positions_near(rootPosition, *m_warpPosition, WARP_TOLERANCE))
        {
            m_lastRootPosition = *m_warpPosition;
            return std::nullopt;
        }

        if (!m_lastRootPosition)
        {
            m_lastRootPosition = rootPosition;
            return std::nullopt;
        }

        const glm::vec2 delta = rootPosition - *m_lastRootPosition;
        m_lastRootPosition = rootPosition;

        if (std::abs(delta.x) > m_bounds.x * DISCONTINUITY_FRACTION ||
            std::abs(delta.y) > m_bounds.y * DISCONTINUITY_FRACTION)
        {
            // Focus changes and independently controlled application cursors
            // can jump discontinuously; resynchronize instead of integrating.
            if (discontinuityPosition)
            {
                const glm::vec2 previous = m_position;
                m_position = clamp_to_bounds(*discontinuityPosition);
                if (m_position != previous)
                    return m_position;
            }
            return std::nullopt;
        }

        const glm::vec2 previous = m_position;
        m_position = clamp_to_bounds(m_position + delta);
        if (m_position == previous)
            return std::nullopt;

        return m_position;
    }

    glm::vec2 VirtualMouseCursor::observe_relative_motion(glm::vec2 delta)
    {
        m_position = clamp_to_bounds(m_position + delta);
        return m_position;
    }

    glm::vec2 VirtualMouseCursor::observe_absolute_motion(glm::vec2 position)
    {
        m_position = clamp_to_bounds(position);
        return m_position;
    }

    glm::vec2 VirtualMouseCursor::get_position() const
    {
        return m_position;
    }

    glm::vec2 VirtualMouseCursor::clamp_to_bounds(glm::vec2 position) const
    {
        return {
            std::clamp(position.x, 0.0f, m_bounds.x),
            std::clamp(position.y, 0.0f, m_bounds.y),
        };
    }
} // namespace vkShade
