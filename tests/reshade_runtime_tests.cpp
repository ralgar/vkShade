#include <chrono>
#include <cstdlib>

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
    REQUIRE(first.frameTime == Catch::Approx(16.0f));
    REQUIRE(first.frameCount == 0);
    REQUIRE(first.timer == Catch::Approx(16.0f));

    // Every effect rendered in this presentation observes one shared sample.
    REQUIRE(runtime.current_frame().frameCount == first.frameCount);
    REQUIRE(runtime.current_frame().frameTime == first.frameTime);

    const auto second = runtime.begin_frame(start + 36ms);
    REQUIRE(second.frameTime == Catch::Approx(20.0f));
    REQUIRE(second.frameCount == 1);
    REQUIRE(second.timer == Catch::Approx(36.0f));
}

TEST_CASE("ReShade generated uniforms use the reference defaults")
{
    vkShade::ReshadePingPongState pingPong;
    pingPong.advance(1.0f, pingPong.stepMin);

    // With ReShade's implicit max=1, the minimum 0.05/s increment moves
    // forward instead of immediately bouncing off an implicit max=0.
    REQUIRE(pingPong.value[0] == Catch::Approx(0.05f));
    REQUIRE(pingPong.value[1] >= 0.0f);

    pingPong.stepMin = 0.5f;
    pingPong.stepMax = 1.0f;
    REQUIRE(pingPong.next_step(2) == Catch::Approx(1.0f));

    const vkShade::ReshadeRandomRange random;
    REQUIRE(random.min == 0);
    REQUIRE(random.max == RAND_MAX);
    REQUIRE(random.value(42) == 42);

    REQUIRE(vkShade::reshade_uniform_uses_initializer(""));
    REQUIRE_FALSE(vkShade::reshade_uniform_uses_initializer("addon_defined"));
}
