#include <chrono>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "vk/reshade_runtime.hpp"

using namespace std::chrono_literals;

TEST_CASE("ReShade frame time is expressed in milliseconds")
{
    REQUIRE(vkShade::reshade_frame_time(16ms) == Catch::Approx(16.0f));
}
