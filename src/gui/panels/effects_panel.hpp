#pragma once
#include "panel.hpp"

#include <unordered_set>

#include <imgui.h>

#include "config/config_store.hpp"
#include "core/event_bus.hpp"
#include "core/uniform.hpp"

namespace vkShade
{
    // Main GUI window
    class EffectsPanel : public GuiPanel
    {
    public:
        EffectsPanel();

        void render() override;

    private:
        ConfigStore& m_config;
        ConfigStore& m_preset;
        EventBus&    m_eventBus;

        // UI-only state (widget selections, window position)
        int32_t m_selectedAvailable = -1;
        int32_t m_selectedActive = -1;
        std::string m_selectedActiveByName;
        ImVec2 m_windowPos = ImVec2(100, 100);
        ImVec2 m_windowSize = ImVec2(600, 450);

        void render_effect_lists();

        // List rendering helper
        bool render_effect_listbox(const char* label,
                                   const std::vector<std::string>& effects,
                                   int32_t& selected,
                                   const ImVec2& size,
                                   const std::unordered_set<std::string>& flaggedEffects = {});

        void render_uniform_controls();

        void render_uniform_button(const Uniform& uniform);
        void render_uniform_color(const Uniform& uniform);
        void render_uniform_combo(const Uniform& uniform);
        void render_uniform_drag(const Uniform& uniform);
        void render_uniform_input(const Uniform& uniform);
        void render_uniform_radio(const Uniform& uniform);
        void render_uniform_slider(const Uniform& uniform);
        void render_uniform_step_control(const Uniform& uniform);

        void step_component(const Uniform& uniform, int32_t direction);

        template<typename T>
        static T get_ui_value(const std::optional<Uniform::Scalar>& value, T fallback)
        {
            if (!value)
                return fallback;

            return std::visit([](auto value) -> T
            {
                return static_cast<T>(value);
            }, *value);
        }
    };
} // namespace vkShade
