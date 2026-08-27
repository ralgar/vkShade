#pragma once

#include <memory>

#include <imgui.h>

#include "config/config_store.hpp"
#include "core/effects_state.hpp"
#include "../panels/effects_panel.hpp"
#include "../panels/log_panel.hpp"
#include "../window.hpp"
#include "about_window.hpp"

namespace vkShade
{
    // Main GUI window
    class MainWindow : public GuiWindow
    {
    public:
        explicit MainWindow(std::shared_ptr<EffectsState> effectsState);

        void render() override;

    private:
        ConfigStore& m_preset;

        // Panels
        EffectsPanel m_effectsPanel;
        LogPanel     m_logPanel;

        // Sub-windows
        AboutWindow m_aboutWindow;

        void render_menu_bar();
    };
} // namespace vkShade
