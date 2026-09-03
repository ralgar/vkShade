#include "runtime/effect_reload_state.hpp"

#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>

using namespace vkShade;

TEST_CASE("Effect reload waits for GPU resources to become idle", "[runtime][reload]")
{
    EffectReloadState reload;
    std::vector<std::string> appliedEffects;
    uint32_t applyCount = 0;

    reload.request({"Old.fx"});
    reload.request({"Latest.fx"});

    CHECK_FALSE(reload.apply_if_safe(VK_TIMEOUT, [&](const auto& effects)
    {
        appliedEffects = effects;
        applyCount++;
    }));
    CHECK(reload.pending());
    CHECK(applyCount == 0);

    CHECK(reload.apply_if_safe(VK_SUCCESS, [&](const auto& effects)
    {
        appliedEffects = effects;
        applyCount++;
    }));
    CHECK_FALSE(reload.pending());
    CHECK(applyCount == 1);
    REQUIRE(appliedEffects.size() == 1);
    CHECK(appliedEffects.front() == "Latest.fx");
}

TEST_CASE("Effect reload preserves requests made while applying", "[runtime][reload]")
{
    EffectReloadState reload;
    std::vector<std::string> appliedEffects;

    reload.request({"Current.fx"});
    REQUIRE(reload.apply_if_safe(VK_SUCCESS, [&](const auto& effects)
    {
        appliedEffects = effects;
        reload.request({"Next.fx"});
    }));

    CHECK(reload.pending());
    REQUIRE(appliedEffects.size() == 1);
    CHECK(appliedEffects.front() == "Current.fx");

    REQUIRE(reload.apply_if_safe(VK_SUCCESS, [&](const auto& effects)
    {
        appliedEffects = effects;
    }));
    CHECK_FALSE(reload.pending());
    REQUIRE(appliedEffects.size() == 1);
    CHECK(appliedEffects.front() == "Next.fx");
}
