#include "xinput_raw_motion.hpp"

namespace vkShade
{
    glm::vec2 decode_xinput_relative_motion(
        std::span<const unsigned char> valuatorMask,
        const double* values)
    {
        glm::vec2 motion {0.0f, 0.0f};
        size_t valueIndex = 0;

        for (size_t axis = 0; axis < valuatorMask.size() * 8; ++axis)
        {
            const bool present =
                (valuatorMask[axis / 8] & (1U << (axis % 8))) != 0;
            if (!present)
                continue;

            // XI2 packs values densely for set mask bits, so the value index
            // cannot be derived directly from the sparse axis number.
            if (axis < 2)
                motion[axis] = static_cast<float>(values[valueIndex]);
            ++valueIndex;
        }

        return motion;
    }

    bool should_forward_xinput_wheel(
        int button,
        bool sourceEvent,
        bool pressed,
        bool rawFallbackActive,
        bool wheelObserverActive)
    {
        return sourceEvent && pressed
            && (rawFallbackActive || wheelObserverActive)
            && button >= 4 && button <= 7;
    }
} // namespace vkShade
