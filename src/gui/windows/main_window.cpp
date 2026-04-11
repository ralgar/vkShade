#include "main_window.hpp"

#include "config/config_manager.hpp"
#include "core/service_locator.hpp"
#include "../gui_helpers.hpp"
#include "../gui_style.hpp"

vkShade::MainWindow::MainWindow()
    : m_config(vkShade::Locator<ConfigManager>::get().app())
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

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        render_effect_lists();

        ImGui::Spacing();

        render_uniform_controls();
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

    ImGui::PopStyleVar();  // WindowPadding
}

void vkShade::MainWindow::render_effect_lists()
{
    auto& internalCfg = vkShade::Locator<vkShade::ConfigManager>::get().internal();

    // Get actually-loaded effects from internal config
    std::vector<std::string> loadedEffects;
    auto loadedEffectsOpt = internalCfg.get<std::vector<std::string>>("__INTERNAL__", "LoadedEffects");
    loadedEffects = loadedEffectsOpt.value_or(std::vector<std::string>{});

    // Get configured/requested effects (what the user wants active)
    std::vector<std::string> activeEffects;
    auto activeEffectsOpt = m_config.get<std::vector<std::string>>("vkShade", "Effects");
    activeEffects = activeEffectsOpt.value_or(std::vector<std::string>{});

    // Build a set of loaded effect names for fast lookup
    std::unordered_set<std::string> loadedSet(loadedEffects.begin(), loadedEffects.end());

    // Scan directory for all effects
    auto effectPaths = m_config.get<std::vector<std::string>>("ReShade", "EffectSearchPaths");
    std::vector<std::string> allEffects;

    namespace fs = std::filesystem;
    for (auto& path : effectPaths.value_or(std::vector<std::string>{}))
    {
        if (fs::exists(path) && fs::is_directory(path))
        {
            for (const auto& entry : fs::directory_iterator(path))
            {
                if (entry.is_regular_file() && entry.path().filename().string().ends_with(".fx"))
                {
                    allEffects.push_back(entry.path().filename().string());
                }
            }
        }
    }
    std::sort(allEffects.begin(), allEffects.end());

    // Filter out active ones to get available
    std::vector<std::string> availableEffects;
    for (const auto& effect : allEffects)
    {
        if (std::find(activeEffects.begin(), activeEffects.end(), effect) == activeEffects.end())
        {
            availableEffects.push_back(effect);
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

    // Available Effects column
    {
        ImGui::BeginChild("AvailableColumn", ImVec2(listWidth, listHeight), false);
        ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "Available Effects");
        ImGui::Spacing();

        // Content section (scrollable)
        {
            float headerHeight = ImGui::GetCursorPosY();
            ImGui::BeginChild("AvailableScrollRegion",
                              ImVec2(0, listHeight - headerHeight - moveButtonsHeight),
                              false,
                              ImGuiWindowFlags_None);

            if (render_effect_listbox("##AvailableEffects", availableEffects, m_selectedAvailable, ImVec2(-FLT_MIN, -FLT_MIN)))
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
                           m_selectedAvailable < (int32_t)availableEffects.size();

        ImVec2 btnSize(-FLT_MIN, UIStyle::BUTTON_HEIGHT);

        if (UI::Button(">>##Activate", btnSize, canActivate, "Activate selected effect"))
        {
            std::string effect = availableEffects[m_selectedAvailable];
            // Add to active list
            activeEffects.push_back(effect);
            m_config.set("vkShade", "Effects", activeEffects);
            m_selectedAvailable = -1;
        }

        bool canDeactivate = m_selectedActive >= 0 &&
                             m_selectedActive < (int32_t)activeEffects.size();

        if (UI::Button("<<##Deactivate", btnSize, canDeactivate, "Deactivate selected effect"))
        {
            // Remove from active list
            activeEffects.erase(activeEffects.begin() + m_selectedActive);
            m_config.set("vkShade", "Effects", activeEffects);
            m_selectedActive = -1;
        }

        ImGui::EndChild();
    }

    ImGui::SameLine();

    // Active Effects column
    {
        ImGui::BeginChild("ActiveColumn", ImVec2(listWidth, listHeight), false);
        ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "Active Effects (Ordered)");
        ImGui::Spacing();

        std::unordered_set<std::string> flaggedEffects;
        for (const auto& effect : activeEffects)
            if (!loadedSet.count(effect))
                flaggedEffects.insert(effect);

        // Content section (scrollable)
        {
            float headerHeight = ImGui::GetCursorPosY();
            ImGui::BeginChild("ActiveScrollRegion",
                              ImVec2(0, listHeight - headerHeight - moveButtonsHeight),
                              false,
                              ImGuiWindowFlags_None);

            if (render_effect_listbox("##ActiveEffects", activeEffects, m_selectedActive,
                                      ImVec2(-FLT_MIN, -FLT_MIN), flaggedEffects))
                m_selectedAvailable = -1;   // When we select in this listbox, deselect in the other.

            ImGui::EndChild();
        }

        // Move Up/Down buttons, centered under the active list
        ImGui::Spacing();
        bool canMoveUp   = m_selectedActive > 0;
        bool canMoveDown = m_selectedActive >= 0 &&
                           m_selectedActive < (int32_t)activeEffects.size() - 1;

        float btnWidth   = 80.0f;
        float totalWidth = btnWidth * 2 + ImGui::GetStyle().ItemSpacing.x;
        float indent     = (listWidth - totalWidth) * 0.5f;
        if (indent > 0) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + indent);

        if (UI::Button("Move Up", ImVec2(btnWidth, 0), canMoveUp))
        {
            std::swap(activeEffects[m_selectedActive], activeEffects[m_selectedActive - 1]);
            m_config.set("vkShade", "Effects", activeEffects);
            m_selectedActive--;
        }

        ImGui::SameLine();

        if (UI::Button("Move Down", ImVec2(btnWidth, 0), canMoveDown))
        {
            std::swap(activeEffects[m_selectedActive], activeEffects[m_selectedActive + 1]);
            m_config.set("vkShade", "Effects", activeEffects);
            m_selectedActive++;
        }

        ImGui::EndChild();
    }

    ImGui::EndGroup();
}

void vkShade::MainWindow::render_uniform_controls()
{
    ImGui::Separator();
    ImGui::Spacing();

    const char* text = "Effect uniforms will be adjustable here";

    float availHeight = ImGui::GetContentRegionAvail().y;
    float textHeight = ImGui::CalcTextSize(text).y;
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (availHeight - textHeight) * 0.5f);

    float textWidth = ImGui::CalcTextSize(text).x;
    ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - textWidth) * 0.5f + ImGui::GetCursorPosX());

    ImGui::TextDisabled("%s", text);
}

bool vkShade::MainWindow::render_effect_listbox(const char* label,
                                                const std::vector<std::string>& effects,
                                                int32_t& selected,
                                                const ImVec2& size,
                                                const std::unordered_set<std::string>& flaggedEffects)
{
    bool changed = false;

    if (ImGui::BeginListBox(label, size))
    {
        for (int32_t i = 0; i < (int32_t)effects.size(); i++)
        {
            const bool isSelected = (selected == i);
            const bool isFlagged = flaggedEffects.count(effects[i]);

            // Grey out items that are flagged
            if (isFlagged)
                ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));

            // Prepend (!) to the display label if flagged
            std::string displayLabel = isFlagged ? "(!) " + effects[i] : effects[i];

            if (ImGui::Selectable(displayLabel.c_str(), isSelected))
            {
                selected = i;
                changed = true;
            }

            if (isFlagged)
                ImGui::PopStyleColor();

            // Set the initial focus when opening the combo (scrolling + keyboard navigation)
            if (isSelected)
                ImGui::SetItemDefaultFocus();

            if (ImGui::IsItemHovered())
            {
                if (isFlagged)
                    ImGui::SetTooltip("This effect failed to load or is not currently active");

                if (ImGui::IsMouseDoubleClicked(0))
                {
                    // TODO: Could trigger activation/deactivation here
                }
            }
        }

        ImGui::EndListBox();
    }

    return changed;
}
