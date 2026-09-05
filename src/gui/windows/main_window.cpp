#include "main_window.hpp"

#include <imgui.h>
#include <ImGuiFileDialog.h>

#include "config/config_manager.hpp"
#include "core/service_locator.hpp"

vkShade::MainWindow::MainWindow()
    : m_preset(vkShade::Locator<ConfigManager>::get().preset())
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
                IGFD::FileDialogConfig config;
                config.path = ".";
                config.flags = ImGuiFileDialogFlags_Modal;
                config.countSelectionMax = 1;
                ImGuiFileDialog::Instance()->OpenDialog(
                    "OpenPreset", "Open Preset", "Preset Files{.ini}", config);
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Save", nullptr, nullptr, m_preset.has_file()))
            {
                m_preset.save();
            }

            if (ImGui::MenuItem("Save As", nullptr, nullptr, true))
            {
                IGFD::FileDialogConfig config;
                config.path = ".";
                config.flags = ImGuiFileDialogFlags_Modal;
                config.countSelectionMax = 1;
                ImGuiFileDialog::Instance()->OpenDialog(
                    "SavePreset", "Save Preset", "Preset Files{.ini}", config);
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Close"))
            {
                m_visible = false;
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

    // Draw the file dialogs and define their handlers
    if (ImGuiFileDialog::Instance()->Display("OpenPreset", ImGuiWindowFlags_NoCollapse, ImVec2(600, 400)))
    {
        if (ImGuiFileDialog::Instance()->IsOk())
        {
            std::string filePath = ImGuiFileDialog::Instance()->GetFilePathName();
            m_preset.load(filePath);
        }

        ImGuiFileDialog::Instance()->Close();
    }

    if (ImGuiFileDialog::Instance()->Display("SavePreset", ImGuiWindowFlags_NoCollapse, ImVec2(600, 400)))
    {
        if (ImGuiFileDialog::Instance()->IsOk())
        {
            std::string filePath = ImGuiFileDialog::Instance()->GetFilePathName();
            m_preset.save(filePath);
        }

        ImGuiFileDialog::Instance()->Close();
    }

    ImGui::PopStyleVar();  // WindowPadding
}
