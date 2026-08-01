#include <chrono>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "vk/reshade_runtime.hpp"

using namespace std::chrono_literals;

TEST_CASE("ReShade frame time is expressed in milliseconds")
{
    REQUIRE(vkShade::reshade_frame_time(16ms) == Catch::Approx(16.0f));
}

TEST_CASE("ReShade runtime values are sampled once per presented frame")
{
    using Clock = std::chrono::steady_clock;
    const Clock::time_point start {};
    vkShade::ReshadeRuntime runtime(start);

    const auto first = runtime.begin_frame(start + 16ms);
    REQUIRE(first.frame_time == Catch::Approx(16.0f));
    REQUIRE(first.frame_count == 0);
    REQUIRE(first.timer == Catch::Approx(16.0f));

    // Every effect rendered in this presentation observes one shared sample.
    REQUIRE(runtime.current_frame().frame_count == first.frame_count);
    REQUIRE(runtime.current_frame().frame_time == first.frame_time);

    const auto second = runtime.begin_frame(start + 36ms);
    REQUIRE(second.frame_time == Catch::Approx(20.0f));
    REQUIRE(second.frame_count == 1);
    REQUIRE(second.timer == Catch::Approx(36.0f));
}
