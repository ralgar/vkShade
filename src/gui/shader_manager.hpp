#pragma once

#include <imgui.h>

#include "config/config_store.hpp"

namespace vkShade
{
    // Main shader manager UI
    class ShaderManagerUI
    {
    public:
        ShaderManagerUI();

        void render();

    private:
        ConfigStore& m_config;

        // UI-only state (widget selections, window position)
        int32_t m_selectedAvailable = -1;
        int32_t m_selectedActive = -1;
        bool m_showWindow = true;
        bool m_showAbout = false;
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
