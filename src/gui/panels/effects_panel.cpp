#include "effects_panel.hpp"

#include "config/config_manager.hpp"
#include "core/events/reload_effects.hpp"
#include "core/service_locator.hpp"
#include "../gui_helpers.hpp"
#include "../gui_style.hpp"
#include "imgui.h"

vkShade::EffectsPanel::EffectsPanel()
    : m_config(vkShade::Locator<ConfigManager>::get().app()),
      m_preset(vkShade::Locator<ConfigManager>::get().preset()),
      m_eventBus(vkShade::Locator<EventBus>::get())
{}

void vkShade::EffectsPanel::render()
{
    ImVec2 contentSize = ImGui::GetContentRegionAvail();

    float uniformHeight = contentSize.y * 0.5f;
    float effectsHeight = contentSize.y - uniformHeight - ImGui::GetStyle().ItemSpacing.y * 2;

    // Effects section
    ImGui::BeginChild("EffectsSection", ImVec2(0, effectsHeight), false);
    render_effect_lists();
    ImGui::EndChild();

    // Uniforms section
    ImGui::BeginChild("UniformsSection", ImVec2(0, uniformHeight), false);
    render_uniform_controls();
    ImGui::EndChild();
}

void vkShade::EffectsPanel::render_effect_lists()
{
    auto& internalCfg = vkShade::Locator<vkShade::ConfigManager>::get().internal();

    // Get actually-loaded effects from internal config
    std::vector<std::string> loadedEffects;
    auto loadedEffectsOpt = internalCfg.get<std::vector<std::string>>("__INTERNAL__", "LoadedEffects");
    loadedEffects = loadedEffectsOpt.value_or(std::vector<std::string>{});

    // Get configured/requested effects (what the user wants active)
    std::vector<std::string> activeEffects;
    auto activeEffectsOpt = m_preset.get<std::vector<std::string>>("", "Effects");
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

    // Draw the widget
    // We use a table to keep the layout aligned
    if (ImGui::BeginTable("EffectsLayout", 3, ImGuiTableFlags_SizingStretchSame))
    {
        ImGui::TableSetupColumn("Available", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableSetupColumn("Active", ImGuiTableColumnFlags_WidthStretch);

        // Row 0: Header
        {
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);

            ImGui::AlignTextToFramePadding();
            ImGui::TextDisabled("Available Effects");

            // Nothing in column 1

            ImGui::TableSetColumnIndex(2);

            ImGui::AlignTextToFramePadding();
            ImGui::TextDisabled("Active Effects");

            ImGui::SameLine();
            float buttonWidth = ImGui::CalcTextSize("Reload").x + ImGui::GetStyle().FramePadding.x * 2.0f;
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - buttonWidth);

            if (ImGui::Button("Reload"))
            {
                Events::ReloadEffects event {};
                m_eventBus.enqueue(event);
            }
        }

        // Row 1: Effect Lists
        {
            ImGui::TableNextRow();

            // We subtract the button height from the listbox Y, to reserve space for row 2 buttons.
            float buttonHeight = UIStyle::BUTTON_HEIGHT + ImGui::GetStyle().FramePadding.y * 2;

            // Available Effects Cell
            ImGui::TableSetColumnIndex(0);
            {
                if (render_effect_listbox("##AvailableEffects", availableEffects, m_selectedAvailable,
                                          ImVec2(-FLT_MIN, ImGui::GetContentRegionAvail().y - buttonHeight)))
                    m_selectedActive = -1;  // When we select in this listbox, deselect in the other.
            }

            // Center Buttons Cell
            ImGui::TableSetColumnIndex(1);
            {
                // Add vertical centering
                float buttonRegionHeight = UIStyle::BUTTON_HEIGHT * 2 + UIStyle::ITEM_SPACING_Y;
                float verticalOffset = (ImGui::GetContentRegionAvail().y - buttonRegionHeight) * 0.5f;
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
                    m_preset.set("", "Effects", activeEffects);
                    m_selectedAvailable = -1;
                }

                bool canDeactivate = m_selectedActive >= 0 &&
                                     m_selectedActive < (int32_t)activeEffects.size();

                if (UI::Button("<<##Deactivate", btnSize, canDeactivate, "Deactivate selected effect"))
                {
                    // Remove from active list
                    activeEffects.erase(activeEffects.begin() + m_selectedActive);
                    m_preset.set("", "Effects", activeEffects);
                    m_selectedActive = -1;
                }
            }

            // Active Effects column
            ImGui::TableSetColumnIndex(2);
            {
                std::unordered_set<std::string> flaggedEffects;
                for (const auto& effect : activeEffects)
                    if (!loadedSet.count(effect))
                        flaggedEffects.insert(effect);

                if (render_effect_listbox("##ActiveEffects", activeEffects, m_selectedActive,
                                          ImVec2(-FLT_MIN, ImGui::GetContentRegionAvail().y - buttonHeight), flaggedEffects))
                    m_selectedAvailable = -1;   // When we select in this listbox, deselect in the other.
            }
        }

        // Row 2: Footer
        {
            ImGui::TableNextRow();

            // Nothing in column 0

            // Nothing in column 1

            // Move Up/Down buttons, centered under the active list
            ImGui::TableSetColumnIndex(2);

            ImGui::Spacing();
            bool canMoveUp   = m_selectedActive > 0;
            bool canMoveDown = m_selectedActive >= 0 &&
                               m_selectedActive < (int32_t)activeEffects.size() - 1;

            float btnWidth   = 80.0f;
            float totalWidth = btnWidth * 2 + ImGui::GetStyle().ItemSpacing.x;
            float indent     = (ImGui::GetContentRegionAvail().x - totalWidth) * 0.5f;
            if (indent > 0) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + indent);

            if (UI::Button("Move Up", ImVec2(btnWidth, 0), canMoveUp))
            {
                std::swap(activeEffects[m_selectedActive], activeEffects[m_selectedActive - 1]);
                m_preset.set("", "Effects", activeEffects);
                m_selectedActive--;
            }

            ImGui::SameLine();

            if (UI::Button("Move Down", ImVec2(btnWidth, 0), canMoveDown))
            {
                std::swap(activeEffects[m_selectedActive], activeEffects[m_selectedActive + 1]);
                m_preset.set("", "Effects", activeEffects);
                m_selectedActive++;
            }

            ImGui::EndTable();
        }
    }
}

void vkShade::EffectsPanel::render_uniform_controls()
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

bool vkShade::EffectsPanel::render_effect_listbox(const char* label,
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
