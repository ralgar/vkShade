#include "input/virtual_mouse_cursor.hpp"

#include <optional>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

namespace
{
    using Catch::Approx;
    using vkShade::VirtualMouseCursor;

    void check_position(const glm::vec2& position, float x, float y)
    {
        CHECK(position.x == Approx(x));
        CHECK(position.y == Approx(y));
    }
}

TEST_CASE("Virtual mouse cursor accumulates root motion within surface bounds")
{
    VirtualMouseCursor cursor({800.0f, 600.0f});
    cursor.reset({400.0f, 300.0f}, {1000.0f, 800.0f});

    const std::optional<glm::vec2> moved = cursor.observe_root_motion({1025.0f, 790.0f});

    REQUIRE(moved);
    check_position(*moved, 425.0f, 290.0f);

    const std::optional<glm::vec2> discontinuity = cursor.observe_root_motion({2025.0f, -210.0f});

    CHECK_FALSE(discontinuity);
    check_position(cursor.get_position(), 425.0f, 290.0f);

    const std::optional<glm::vec2> resumed = cursor.observe_root_motion({2026.0f, -209.0f});

    REQUIRE(resumed);
    check_position(*resumed, 426.0f, 291.0f);
}

TEST_CASE("Virtual mouse cursor ignores application warps and resets their baseline")
{
    VirtualMouseCursor cursor({800.0f, 600.0f});
    cursor.reset({400.0f, 300.0f}, {1000.0f, 800.0f});
    cursor.set_warp_position({1000.0f, 800.0f});

    REQUIRE(cursor.observe_root_motion({1010.0f, 800.0f}));
    check_position(cursor.get_position(), 410.0f, 300.0f);

    CHECK_FALSE(cursor.observe_root_motion({1000.5f, 799.5f}));
    check_position(cursor.get_position(), 410.0f, 300.0f);

    const std::optional<glm::vec2> moved = cursor.observe_root_motion({1005.0f, 800.0f});

    REQUIRE(moved);
    check_position(*moved, 415.0f, 300.0f);
}

TEST_CASE("Virtual mouse cursor resynchronizes an unknown discontinuity")
{
    VirtualMouseCursor cursor({800.0f, 600.0f});
    cursor.reset({400.0f, 300.0f}, {1000.0f, 800.0f});
    cursor.set_warp_position({1000.0f, 800.0f});

    const std::optional<glm::vec2> moved =
        cursor.observe_root_motion(
            {1500.0f, 1100.0f}, glm::vec2 {700.0f, 500.0f});

    REQUIRE(moved);
    check_position(*moved, 700.0f, 500.0f);

    CHECK_FALSE(cursor.observe_root_motion(
        {1000.0f, 800.0f}, glm::vec2 {400.0f, 300.0f}));
    check_position(cursor.get_position(), 700.0f, 500.0f);
}

TEST_CASE("Virtual mouse cursor shares absolute and relative motion state")
{
    VirtualMouseCursor cursor({800.0f, 600.0f});
    cursor.reset({400.0f, 300.0f});

    check_position(cursor.observe_relative_motion({20.0f, -10.0f}), 420.0f, 290.0f);
    check_position(cursor.observe_absolute_motion({100.0f, 700.0f}), 100.0f, 600.0f);
    check_position(cursor.observe_relative_motion({-150.0f, -50.0f}), 0.0f, 550.0f);

    cursor.set_bounds({50.0f, 40.0f});
    check_position(cursor.get_position(), 0.0f, 40.0f);
}
