#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "core/rolling_statistics.hpp"

TEST_CASE("rolling statistics report samples in their active window")
{
    vkShade::RollingStatistics<3> statistics;

    CHECK_FALSE(statistics.snapshot().has_value());

    statistics.add(1.0);
    statistics.add(2.0);
    statistics.add(3.0);

    const auto snapshot = statistics.snapshot();
    REQUIRE(snapshot.has_value());
    CHECK(snapshot->current == Catch::Approx(3.0));
    CHECK(snapshot->average == Catch::Approx(2.0));
    CHECK(snapshot->minimum == Catch::Approx(1.0));
    CHECK(snapshot->maximum == Catch::Approx(3.0));
}

TEST_CASE("rolling statistics replace the oldest sample")
{
    vkShade::RollingStatistics<3> statistics;
    statistics.add(1.0);
    statistics.add(2.0);
    statistics.add(3.0);
    statistics.add(4.0);

    const auto snapshot = statistics.snapshot();
    REQUIRE(snapshot.has_value());
    CHECK(snapshot->current == Catch::Approx(4.0));
    CHECK(snapshot->average == Catch::Approx(3.0));
    CHECK(snapshot->minimum == Catch::Approx(2.0));
    CHECK(snapshot->maximum == Catch::Approx(4.0));
}

TEST_CASE("rolling statistics reset their accumulated state")
{
    vkShade::RollingStatistics<3> statistics;
    statistics.add(1.0);

    statistics.reset();

    CHECK_FALSE(statistics.snapshot().has_value());
}
