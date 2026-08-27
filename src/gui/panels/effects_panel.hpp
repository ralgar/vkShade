#pragma once
#include "panel.hpp"

#include <memory>
#include <unordered_set>

#include <imgui.h>

#include "config/config_store.hpp"
#include "core/effects_state.hpp"
#include "core/event_bus.hpp"

namespace vkShade
{
    // Main GUI window
    class EffectsPanel : public GuiPanel
    {
    public:
        explicit EffectsPanel(std::shared_ptr<EffectsState> effectsState);

        void render() override;

    private:
        ConfigStore& m_config;
        ConfigStore& m_preset;
        EventBus&    m_eventBus;
        std::shared_ptr<EffectsState> m_effectsState;

        // UI-only state (widget selections, window position)
        int32_t m_selectedAvailable = -1;
        int32_t m_selectedActive = -1;
        ImVec2 m_windowPos = ImVec2(100, 100);
        ImVec2 m_windowSize = ImVec2(600, 450);

        void render_controls_bar();
        void render_effect_lists();
        void render_uniform_controls();

        // List rendering helper
        bool render_effect_listbox(const char* label,
                                   const std::vector<std::string>& effects,
                                   int32_t& selected,
                                   const ImVec2& size,
                                   const std::unordered_set<std::string>& flaggedEffects = {});
    };
} // namespace vkShade
