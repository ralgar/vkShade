#pragma once

#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <vulkan/vulkan_core.h>

namespace vkShade
{
    class EffectReloadState
    {
    public:
        void request(std::vector<std::string> effects)
        {
            m_pendingEffects = std::move(effects);
        }

        [[nodiscard]] bool pending() const
        {
            return m_pendingEffects.has_value();
        }

        template<typename Apply>
        bool apply_if_safe(VkResult fenceResult, Apply&& apply)
        {
            if (fenceResult != VK_SUCCESS || !m_pendingEffects)
                return false;

            // Consume before invoking so a request made while applying is not
            // erased as part of completing the older request.
            auto effects = std::move(*m_pendingEffects);
            m_pendingEffects.reset();
            std::invoke(std::forward<Apply>(apply), effects);
            return true;
        }

    private:
        std::optional<std::vector<std::string>> m_pendingEffects;
    };
}  // namespace vkShade
