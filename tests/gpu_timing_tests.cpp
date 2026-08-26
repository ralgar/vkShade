#include <catch2/catch_test_macros.hpp>

#include "vk/gpu_timestamp.hpp"

TEST_CASE("timestamp deltas preserve full-width counters")
{
    CHECK(vkShade::timestamp_tick_delta(100, 175, 64) == 75);
}

TEST_CASE("timestamp deltas handle valid-bit wraparound")
{
    CHECK(vkShade::timestamp_tick_delta(250, 5, 8) == 11);
}
