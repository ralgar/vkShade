#include "main_window.hpp"

#include "config/config_manager.hpp"
#include "core/service_locator.hpp"

vkShade::MainWindow::MainWindow(std::shared_ptr<DiagnosticsState> diagnosticsState,
                                std::shared_ptr<ImageTracker> imageTracker)
    : m_preset(vkShade::Locator<ConfigManager>::get().preset()),
      m_diagnosticsState(std::move(diagnosticsState)),
      m_bufferPanel(std::move(imageTracker))
{}

void vkShade::MainWindow::render()
{
    // Set initial/default size and position
    ImVec2 size = ImGui::GetMainViewport()->Size;
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(size.x * 0.5, size.y * 0.6), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("vkShade", &m_visible, ImGuiWindowFlags_MenuBar))
    {
        render_menu_bar();

        if (ImGui::BeginTabBar("MainTabs"))
        {
            if (ImGui::BeginTabItem("Effects###effects"))
            {
                m_effectsPanel.render();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Buffers###buffers"))
            {
                m_bufferPanel.render();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Logs###logs"))
            {
                m_logPanel.render();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
    }

    if (m_aboutWindow.visible())
        m_aboutWindow.render();

    ImGui::End();
}

void vkShade::MainWindow::render_menu_bar()
{
    // Override custom padding and use the default (much less) for menus
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));

    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("New"))
            {
                m_preset.clear();
            }

            if (ImGui::MenuItem("Open"))
            {
                // TODO: Open file browser and select file
                m_preset.load("ReShade.ini");  // From CWD
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Save", nullptr, nullptr, true))
            {
                if (m_preset.has_file())
                    m_preset.save();
                else
                    m_preset.save("ReShade.ini");  // In CWD
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Close"))
            {
                m_visible = false;
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View"))
        {
            bool showPerformance = m_diagnosticsState->showPerformanceOverlay.load(
                std::memory_order_relaxed);
            if (ImGui::MenuItem("Performance overlay", nullptr, &showPerformance))
            {
                m_diagnosticsState->showPerformanceOverlay.store(
                    showPerformance, std::memory_order_relaxed);
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help"))
        {
            if (ImGui::MenuItem("About"))
            {
                m_aboutWindow.visible(true);
            }

            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }

    ImGui::PopStyleVar();  // WindowPadding
}
