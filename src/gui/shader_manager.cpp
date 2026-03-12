#include "shader_manager.hpp"

#include "config/config_globals.hpp"
#include "config/config_manager.hpp"
#include "core/service_locator.hpp"
#include "gui_helpers.hpp"
#include "gui_style.hpp"

vkShade::ShaderManagerUI::ShaderManagerUI()
    : m_config(vkShade::Locator<ConfigManager>::get().app())
{}

void vkShade::ShaderManagerUI::render()
{
    if (!m_showWindow)
        return;

    if (ImGui::Begin("vkShade: Shader Manager", &m_showWindow, ImGuiWindowFlags_MenuBar))
    {
        render_menu_bar();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        render_shader_lists();

        ImGui::Spacing();

        render_uniform_controls();
    }

    ImGui::End();
}

void vkShade::ShaderManagerUI::render_about_dialog()
{
    if (!m_showAbout)
        return;

    ImGui::SetNextWindowSize(ImVec2(300, 120), ImGuiCond_Always);
    if (ImGui::Begin("About vkShade", &m_showAbout, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse))
    {
        ImGui::Text("vkShade");
        ImGui::Spacing();
        ImGui::TextDisabled("A Vulkan post-processing shader manager");
        ImGui::Spacing();
        ImGui::TextDisabled("Version 0.1.0");
    }
    ImGui::End();
}

void vkShade::ShaderManagerUI::render_menu_bar()
{
    // Override custom padding and use the default (much less) for menus
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));

    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("New"))
            {
                m_config.clear();
            }

            if (ImGui::MenuItem("Open"))
            {
                // TODO: Open file browser and select file
                m_config.load("vkShade.ini");  // From CWD
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Save", nullptr, nullptr, m_config.has_file()))
            {
                m_config.save();
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Close"))
            {
                m_showWindow = false;
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help"))
        {
            if (ImGui::MenuItem("About"))
            {
                // TODO: Open a simple about dialog
            }

            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }

    ImGui::PopStyleVar();  // WindowPadding
}

void vkShade::ShaderManagerUI::render_shader_lists()
{
    // Get active shaders from config
    std::vector<std::string> activeShaders;
    auto activeShadersOpt = m_config.get<std::vector<std::string>>("vkShade", "Effects");
    activeShaders = activeShadersOpt.value_or(std::vector<std::string>{});

    // Scan directory for all shaders
    std::string effectsPath = std::string(DATADIR) + "/vkShade";
    std::vector<std::string> allShaders;

    namespace fs = std::filesystem;
    if (fs::exists(effectsPath) && fs::is_directory(effectsPath))
    {
        for (const auto& entry : fs::directory_iterator(effectsPath))
        {
            if (entry.is_regular_file() && entry.path().filename().string().ends_with(".frag.spv"))
            {
                allShaders.push_back(entry.path().filename().string());
            }
        }
    }
    std::sort(allShaders.begin(), allShaders.end());

    // Filter out active ones to get available
    std::vector<std::string> availableShaders;
    for (const auto& shader : allShaders)
    {
        if (std::find(activeShaders.begin(), activeShaders.end(), shader) == activeShaders.end())
        {
            availableShaders.push_back(shader);
        }
    }

    // Calculate available space for list boxes
    float availableWidth = ImGui::GetContentRegionAvail().x;
    float availableHeight = ImGui::GetContentRegionAvail().y;

    // Reserve space for control buttons section (separator + spacing + buttons + spacing + info text)
    float controlButtonsHeight = UIStyle::ITEM_SPACING_Y * 3 + UIStyle::BUTTON_HEIGHT + 20.0f; // adjust as needed
    float listHeight = availableHeight - controlButtonsHeight;

    if (listHeight < UIStyle::LISTBOX_MIN_HEIGHT)
        listHeight = UIStyle::LISTBOX_MIN_HEIGHT;

    float listWidth = (availableWidth - UIStyle::ITEM_SPACING_X * 2 - 80.0f) * 0.5f;

    float moveButtonsHeight = UIStyle::BUTTON_HEIGHT + ImGui::GetStyle().ItemSpacing.y * 2;

    ImGui::BeginGroup();

    // Available Shaders column
    {
        ImGui::BeginChild("AvailableColumn", ImVec2(listWidth, listHeight), false);
        ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "Available Shaders");
        ImGui::Spacing();

        // Content section (scrollable)
        {
            float headerHeight = ImGui::GetCursorPosY();
            ImGui::BeginChild("AvailableScrollRegion",
                              ImVec2(0, listHeight - headerHeight - moveButtonsHeight),
                              false,
                              ImGuiWindowFlags_None);

            if (render_shader_listbox("##AvailableShaders", availableShaders, m_selectedAvailable, ImVec2(-FLT_MIN, -FLT_MIN)))
                m_selectedActive = -1;  // When we select in this listbox, deselect in the other.

            ImGui::EndChild();
        }

        ImGui::EndChild();
    }

    ImGui::SameLine();

    // Center buttons column
    {
        ImGui::BeginChild("ButtonsColumn", ImVec2(80.0f, listHeight), false);

        // Add vertical centering
        float buttonRegionHeight = UIStyle::BUTTON_HEIGHT * 2 + UIStyle::ITEM_SPACING_Y;
        float verticalOffset = (listHeight - moveButtonsHeight - buttonRegionHeight) * 0.5f;
        if (verticalOffset > 0) {
            ImGui::Dummy(ImVec2(0, verticalOffset));
        }

        bool canActivate = m_selectedAvailable >= 0 &&
                           m_selectedAvailable < (int)availableShaders.size();

        ImVec2 btnSize(-FLT_MIN, UIStyle::BUTTON_HEIGHT);

        if (UI::Button(">>##Activate", btnSize, canActivate, "Activate selected shader"))
        {
            std::string shader = availableShaders[m_selectedAvailable];
            // Add to active list
            activeShaders.push_back(shader);
            m_config.set("vkShade", "Effects", activeShaders);
            m_selectedAvailable = -1;
        }

        bool canDeactivate = m_selectedActive >= 0 &&
                             m_selectedActive < (int)activeShaders.size();

        if (UI::Button("<<##Deactivate", btnSize, canDeactivate, "Deactivate selected shader"))
        {
            // Remove from active list
            activeShaders.erase(activeShaders.begin() + m_selectedActive);
            m_config.set("vkShade", "Effects", activeShaders);
            m_selectedActive = -1;
        }

        ImGui::EndChild();
    }

    ImGui::SameLine();

    // Active Shaders column
    {
        ImGui::BeginChild("ActiveColumn", ImVec2(listWidth, listHeight), false);
        ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "Active Shaders (Ordered)");
        ImGui::Spacing();

        // Content section (scrollable)
        {
            float headerHeight = ImGui::GetCursorPosY();
            ImGui::BeginChild("ActiveScrollRegion",
                              ImVec2(0, listHeight - headerHeight - moveButtonsHeight),
                              false,
                              ImGuiWindowFlags_None);

            if (render_shader_listbox("##ActiveShaders", activeShaders, m_selectedActive, ImVec2(-FLT_MIN, -FLT_MIN)))
                m_selectedAvailable = -1;   // When we select in this listbox, deselect in the other.

            ImGui::EndChild();
        }

        // Move Up/Down buttons, centered under the active list
        ImGui::Spacing();
        bool canMoveUp   = m_selectedActive > 0;
        bool canMoveDown = m_selectedActive >= 0 &&
                           m_selectedActive < (int)activeShaders.size() - 1;

        float btnWidth   = 80.0f;
        float totalWidth = btnWidth * 2 + ImGui::GetStyle().ItemSpacing.x;
        float indent     = (listWidth - totalWidth) * 0.5f;
        if (indent > 0) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + indent);

        if (UI::Button("Move Up", ImVec2(btnWidth, 0), canMoveUp))
        {
            std::swap(activeShaders[m_selectedActive], activeShaders[m_selectedActive - 1]);
            m_config.set("vkShade", "Effects", activeShaders);
            m_selectedActive--;
        }

        ImGui::SameLine();

        if (UI::Button("Move Down", ImVec2(btnWidth, 0), canMoveDown))
        {
            std::swap(activeShaders[m_selectedActive], activeShaders[m_selectedActive + 1]);
            m_config.set("vkShade", "Effects", activeShaders);
            m_selectedActive++;
        }

        ImGui::EndChild();
    }

    ImGui::EndGroup();
}

void vkShade::ShaderManagerUI::render_uniform_controls()
{
    ImGui::Separator();
    ImGui::Spacing();

    const char* text = "Shader uniforms will be adjustable here";

    float availHeight = ImGui::GetContentRegionAvail().y;
    float textHeight = ImGui::CalcTextSize(text).y;
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (availHeight - textHeight) * 0.5f);

    float textWidth = ImGui::CalcTextSize(text).x;
    ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - textWidth) * 0.5f + ImGui::GetCursorPosX());

    ImGui::TextDisabled("%s", text);
}

bool vkShade::ShaderManagerUI::render_shader_listbox(const char* label,
                                                     const std::vector<std::string>& shaders,
                                                     int& selected,
                                                     const ImVec2& size)
{
    bool changed = false;

    if (ImGui::BeginListBox(label, size))
    {
        for (int i = 0; i < (int)shaders.size(); i++)
        {
            const bool isSelected = (selected == i);

            if (ImGui::Selectable(shaders[i].c_str(), isSelected))
            {
                selected = i;
                changed = true;
            }

            // Set the initial focus when opening the combo (scrolling + keyboard navigation)
            if (isSelected)
            {
                ImGui::SetItemDefaultFocus();
            }

            // Double-click to activate/deactivate
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
            {
                // TODO: Could trigger activation/deactivation here
            }
        }

        ImGui::EndListBox();
    }

    return changed;
}
