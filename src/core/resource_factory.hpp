#pragma once

#include <memory>

#include "resource.hpp"

namespace vkShade
{
    // Default factory. Users can specialize if needed.
    template<IsResource T>
    struct ResourceFactory
    {
        template<typename... Args>
        static std::shared_ptr<T> create(Args&&... args)
        {
            return std::make_shared<T>(std::forward<Args>(args)...);
        }
    };
} // namespace vkShade
