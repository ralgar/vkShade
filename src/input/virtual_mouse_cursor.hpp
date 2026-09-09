#pragma once

#include <optional>

#include <glm/vec2.hpp>

namespace vkShade
{
    class VirtualMouseCursor
    {
    public:
        explicit VirtualMouseCursor(glm::vec2 bounds);

        void reset(glm::vec2 position);
        void reset(glm::vec2 position, glm::vec2 rootPosition);
        void set_bounds(glm::vec2 bounds);
        void set_warp_position(glm::vec2 rootPosition);
        void clear_warp_position();

        std::optional<glm::vec2> observe_root_motion(
            glm::vec2 rootPosition,
            std::optional<glm::vec2> discontinuityPosition = std::nullopt);
        glm::vec2 observe_relative_motion(glm::vec2 delta);
        glm::vec2 observe_absolute_motion(glm::vec2 position);

        glm::vec2 get_position() const;

    private:
        glm::vec2 clamp_to_bounds(glm::vec2 position) const;

        glm::vec2 m_bounds {0.0f, 0.0f};
        glm::vec2 m_position {0.0f, 0.0f};
        std::optional<glm::vec2> m_lastRootPosition;
        std::optional<glm::vec2> m_warpPosition;
    };
} // namespace vkShade
