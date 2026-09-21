#pragma once

#include <memory>

namespace vkShade
{
    class MouseInputInhibitor;

    std::unique_ptr<MouseInputInhibitor> create_application_mouse_inhibitor();
} // namespace vkShade
