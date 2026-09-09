#include "input/xinput_raw_motion.hpp"

#include <array>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

namespace
{
    using Catch::Approx;
    using vkShade::decode_xinput_relative_motion;
    using vkShade::should_forward_xinput_wheel;

    void set_axis(std::array<unsigned char, 2>& mask, int axis)
    {
        mask[axis / 8] |= static_cast<unsigned char>(1U << (axis % 8));
    }
}

TEST_CASE("XInput raw motion decodes densely packed X and Y valuators")
{
    std::array<unsigned char, 2> mask {};
    set_axis(mask, 0);
    set_axis(mask, 1);
    set_axis(mask, 9);
    const std::array values {3.5, -2.25, 42.0};

    const glm::vec2 motion = decode_xinput_relative_motion(mask, values.data());

    CHECK(motion.x == Approx(3.5f));
    CHECK(motion.y == Approx(-2.25f));
}

TEST_CASE("XInput raw motion preserves packing when the X axis is absent")
{
    std::array<unsigned char, 1> mask {};
    mask[0] = static_cast<unsigned char>(1U << 1);
    const std::array values {-4.0};

    const glm::vec2 motion = decode_xinput_relative_motion(mask, values.data());

    CHECK(motion.x == Approx(0.0f));
    CHECK(motion.y == Approx(-4.0f));
}

TEST_CASE("XInput wheel routing forwards one physical source event")
{
    CHECK(should_forward_xinput_wheel(4, true, true, true, false));
    CHECK(should_forward_xinput_wheel(7, true, true, false, true));

    CHECK_FALSE(should_forward_xinput_wheel(4, false, true, true, false));
    CHECK_FALSE(should_forward_xinput_wheel(4, true, false, true, false));
    CHECK_FALSE(should_forward_xinput_wheel(4, true, true, false, false));
    CHECK_FALSE(should_forward_xinput_wheel(3, true, true, true, false));
    CHECK_FALSE(should_forward_xinput_wheel(8, true, true, true, false));
}
