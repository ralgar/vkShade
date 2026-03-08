#pragma once

#include <type_traits>

namespace vkShade
{
    template<typename>
    struct always_false : std::false_type {};
}
