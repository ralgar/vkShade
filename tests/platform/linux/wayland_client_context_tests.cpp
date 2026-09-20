#include "platform/linux/wayland_client_context.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Wayland client contexts share the latest input serial")
{
    auto state = std::make_shared<vkShade::Platform::WaylandClientState>(nullptr);
    vkShade::Platform::WaylandClientContext context(state);

    CHECK(context.get_state() == state);
    CHECK(state->get_input_serial() == 0);

    state->set_input_serial(42);

    CHECK(context.get_state()->get_input_serial() == 42);
    CHECK_FALSE(context.is_available());
}
