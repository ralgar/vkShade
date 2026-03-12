#pragma once

#include <imgui.h>

#include "config/config_store.hpp"
#include "../window.hpp"
#include "about_window.hpp"

namespace vkShade
{
    // Main GUI window
    class MainWindow : public GuiWindow
    {
    public:
        MainWindow();

        void render();

    private:
        ConfigStore& m_config;

        // Sub-windows
        AboutWindow m_aboutWindow;

        // UI-only state (widget selections, window position)
        int32_t m_selectedAvailable = -1;
        int32_t m_selectedActive = -1;
        ImVec2 m_windowPos = ImVec2(100, 100);
        ImVec2 m_windowSize = ImVec2(600, 450);

        void render_about_dialog();
        void render_menu_bar();

        void render_shader_lists();
        void render_uniform_controls();

        // List rendering helper
        bool render_shader_listbox(const char* label,
                                   const std::vector<std::string>& shaders,
                                   int32_t& selected,
                                   const ImVec2& size);
    };
} // namespace vkShade
