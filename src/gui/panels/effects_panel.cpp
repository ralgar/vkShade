#include "effects_panel.hpp"

#include "config/config_manager.hpp"
#include "core/events/reload_effects.hpp"
#include "core/service_locator.hpp"
#include "hooks/hooks.hpp"
#include "runtime/runtime.hpp"
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

    float uniformHeight = contentSize.y * 0.55f;
    float effectsHeight = contentSize.y - uniformHeight - ImGui::GetStyle().ItemSpacing.y * 2;

    // Effects section
    ImGui::BeginChild("EffectsSection", ImVec2(0, effectsHeight), false);
    render_effect_lists();
    ImGui::EndChild();

    ImGui::Separator();

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
                {
                    m_selectedActive = -1;  // When we select in this listbox, deselect in the other.
                    m_selectedActiveByName.clear();
                }
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
                    m_selectedActive = activeEffects.size() - 1;
                    m_selectedAvailable = -1;
                    m_selectedActiveByName = effect;
                }

                bool canDeactivate = m_selectedActive >= 0 &&
                                     m_selectedActive < (int32_t)activeEffects.size();

                if (UI::Button("<<##Deactivate", btnSize, canDeactivate, "Deactivate selected effect"))
                {
                    // Remove from active list
                    activeEffects.erase(activeEffects.begin() + m_selectedActive);
                    m_preset.set("", "Effects", activeEffects);
                    m_selectedActive = -1;
                    m_selectedActiveByName.clear();
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
                {
                    m_selectedAvailable = -1;   // When we select in this listbox, deselect in the other.

                    if (m_selectedActive >= 0 &&
                        m_selectedActive < static_cast<int32_t>(activeEffects.size()))
                    {
                        m_selectedActiveByName = activeEffects[m_selectedActive];
                    }
                    else
                    {
                        m_selectedActiveByName.clear();
                    }
                }
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

void vkShade::EffectsPanel::render_uniform_controls()
{
    // FIXME: This is awful but we're already doing it elsewhere and it works for now
    Runtime& runtime = g_runtimes.begin()->second;

    const ReshadeEffect* effect = runtime.get_effect(m_selectedActiveByName);
    if (!effect)
    {
        const char* text = "No effect selected";

        float availHeight = ImGui::GetContentRegionAvail().y;
        float textHeight = ImGui::CalcTextSize(text).y;
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (availHeight - textHeight) * 0.5f);

        float textWidth = ImGui::CalcTextSize(text).x;
        ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - textWidth) * 0.5f + ImGui::GetCursorPosX());

        ImGui::TextDisabled("%s", text);

        return;
    }

    // Reset Button (All Uniforms)
    if (ImGui::Button("Reset All to Default", ImVec2(-FLT_MIN, 0.0f)))
    {
        for (const auto& uniform : effect->uniforms())
        {
            if (uniform.noReset)
                continue;

            Uniform::dispatch_type(uniform.baseType, uniform.components,
                [&]<typename T>(std::type_identity<T>)
            {
                T value {};

                if constexpr (UniformTraits<T>::components == 1)
                {
                    value = std::get<typename UniformTraits<T>::Scalar>(
                        uniform.defaultValues[0].value());
                }
                else
                {
                    for (uint32_t i = 0; i < UniformTraits<T>::components; i++)
                    {
                        value[i] = std::get<typename UniformTraits<T>::Scalar>(
                            uniform.defaultValues[i].value());
                    }
                }

                m_preset.set(m_selectedActiveByName, uniform.name, value);
            });
        }
    }

    constexpr float labelWidth = 180.0f;
    const float resetWidth = ImGui::CalcTextSize("Reset").x + ImGui::GetStyle().FramePadding.x * 2.0f;

    std::string_view currentCategory;
    bool tableOpen = false;
    bool tableIndented = false;
    bool firstUniform = true;
    for (const auto& uniform : effect->uniforms())
    {
        if (firstUniform || uniform.uiCategory != currentCategory)
        {
            firstUniform = false;

            if (tableOpen)
            {
                ImGui::EndTable();

                if (tableIndented)
                {
                    ImGui::Unindent();
                    tableIndented = false;
                }

                tableOpen = false;
            }

            if (!currentCategory.empty())
                ImGui::PopID();

            currentCategory = uniform.uiCategory;

            if (!currentCategory.empty())
            {
                ImGui::PushID(currentCategory.data());

                ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None;

                if (!uniform.uiCategoryClosed)
                    flags |= ImGuiTreeNodeFlags_DefaultOpen;

                if (!ImGui::CollapsingHeader(currentCategory.data(), flags))
                    continue;

                ImGui::Indent();
                tableIndented = true;
            }

            tableOpen = ImGui::BeginTable("Uniforms", 3, ImGuiTableFlags_SizingStretchProp);

            if (tableOpen)
            {
                ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, labelWidth);
                ImGui::TableSetupColumn("Value");
                ImGui::TableSetupColumn("Reset", ImGuiTableColumnFlags_WidthFixed, resetWidth);
            }
        }

        if (uniform.hidden || !tableOpen)
            continue;

        ImGui::PushID(uniform.name.c_str());

        ImGui::BeginDisabled(uniform.disabled);

        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(uniform.uiLabel.c_str());

        ImGui::TableSetColumnIndex(1);

        bool hasStepControl = (*uniform.uiType == Uniform::UiType::Drag ||
                               *uniform.uiType == Uniform::UiType::Slider) &&
                               uniform.components == 1;

        // Subtract width for step controls, if present.
        float stepControlsWidth = 0.0f;
        if (hasStepControl)
        {
            stepControlsWidth = 2 * ImGui::GetFrameHeight()
                + ImGui::GetStyle().ItemSpacing.x + ImGui::GetStyle().CellPadding.x;
        }

        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - stepControlsWidth);

        switch (*uniform.uiType)
        {
            case Uniform::UiType::Input:  render_uniform_input(uniform);  break;
            case Uniform::UiType::Drag:   render_uniform_drag(uniform);   break;
            case Uniform::UiType::Slider: render_uniform_slider(uniform); break;
            case Uniform::UiType::Combo:  render_uniform_combo(uniform);  break;
            case Uniform::UiType::Radio:  render_uniform_radio(uniform);  break;
            case Uniform::UiType::Color:  render_uniform_color(uniform);  break;
            case Uniform::UiType::Button: render_uniform_button(uniform); break;
        }

        // Set the tooltip if we have one
        if (!uniform.uiTooltip.empty())
            ImGui::SetItemTooltip("%s", uniform.uiTooltip.c_str());

        // Draw the step controls if appropriate
        if (hasStepControl)
        {
            ImVec2 spacing = ImGui::GetStyle().ItemSpacing;
            spacing.x /= 2;
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, spacing);

            ImGui::SameLine();
            render_uniform_step_control(uniform);

            ImGui::PopStyleVar();
        }

        ImGui::TableSetColumnIndex(2);

        // Reset Button (Individual Uniform)
        if (!uniform.noReset && ImGui::Button("Reset"))
        {
            Uniform::dispatch_type(uniform.baseType, uniform.components,
                [&]<typename T>(std::type_identity<T>)
            {
                T value {};

                if constexpr (UniformTraits<T>::components == 1)
                {
                    value = std::get<typename UniformTraits<T>::Scalar>(
                        uniform.defaultValues[0].value());
                }
                else
                {
                    for (uint32_t i = 0; i < UniformTraits<T>::components; i++)
                    {
                        value[i] = std::get<typename UniformTraits<T>::Scalar>(
                            uniform.defaultValues[i].value());
                    }
                }

                m_preset.set(m_selectedActiveByName, uniform.name, value);
            });
        }

        ImGui::EndDisabled();

        ImGui::PopID();
    }

    if (tableOpen)
    {
        ImGui::EndTable();

        if (tableIndented)
            ImGui::Unindent();
    }

    if (!currentCategory.empty())
        ImGui::PopID();
}

void vkShade::EffectsPanel::render_uniform_button(const Uniform& uniform)
{
    ImGui::TextColored(UIStyle::Palette::YELLOW, "Unsupported widget: Button");
}

void vkShade::EffectsPanel::render_uniform_color(const Uniform& uniform)
{
    if (uniform.baseType != Uniform::BaseType::Float ||
        (uniform.components != 3 && uniform.components != 4))
    {
        ImGui::TextColored(UIStyle::Palette::RED, "Invalid uniform type for color");
        return;
    }

    Uniform::dispatch_type(uniform.baseType, uniform.components,
        [&]<typename T>(std::type_identity<T>)
    {
        if constexpr (std::is_same_v<typename UniformTraits<T>::Scalar, float> &&
            (UniformTraits<T>::components == 3 || UniformTraits<T>::components == 4))
        {
            auto value = m_preset.get<T>(m_selectedActiveByName, uniform.name);
            if (!value)
            {
                ImGui::TextColored(UIStyle::Palette::RED, "Invalid uniform value");
                return;
            }

            T v = *value;

            ImGuiColorEditFlags flags = ImGuiColorEditFlags_None;

            bool changed = false;
            if constexpr (UniformTraits<T>::components == 3)
                changed = ImGui::ColorEdit3("##value", &v[0], flags);
            else if constexpr (UniformTraits<T>::components == 4)
                changed = ImGui::ColorEdit4("##value", &v[0], flags);

            if (changed)
                m_preset.set(m_selectedActiveByName, uniform.name, v);
        }
    });
}

void vkShade::EffectsPanel::render_uniform_combo(const Uniform& uniform)
{
    ImGui::TextColored(UIStyle::Palette::YELLOW, "Unsupported widget: Combo");
}

void vkShade::EffectsPanel::render_uniform_drag(const Uniform& uniform)
{
    Uniform::dispatch_type(uniform.baseType, uniform.components,
        [&]<typename T>(std::type_identity<T>)
    {
        auto value = m_preset.get<T>(m_selectedActiveByName, uniform.name);
        if (!value)
        {
            ImGui::TextColored(UIStyle::Palette::RED, "Invalid uniform value");
            return;
        }

        T v = *value;

        ImGuiDataType dataType;
        Uniform::Scalar min;
        Uniform::Scalar max;
        std::string format;

        switch (uniform.baseType)
        {
            case Uniform::BaseType::Float:
                dataType = ImGuiDataType_Float;
                min = get_ui_value(uniform.uiMin, 0.0f);
                max = get_ui_value(uniform.uiMax, 1.0f);
                format = "%.3f" + uniform.uiUnits;
                break;

            case Uniform::BaseType::Int:
                dataType = ImGuiDataType_S32;
                min = get_ui_value(uniform.uiMin, int32_t{0});
                max = get_ui_value(uniform.uiMax, int32_t{100});
                format = "%d" + uniform.uiUnits;
                break;

            case Uniform::BaseType::Uint:
                dataType = ImGuiDataType_U32;
                min = get_ui_value(uniform.uiMin, uint32_t{0});
                max = get_ui_value(uniform.uiMax, uint32_t{100});
                format = "%u" + uniform.uiUnits;
                break;

            default:
                ImGui::TextColored(UIStyle::Palette::RED, "Invalid uniform type for slider");
                return;
        }

        float speed = get_ui_value(uniform.uiStep, 0.0f);
        if (ImGui::DragScalarN("##value", dataType, &v, uniform.components, speed, &min, &max, format.c_str()))
            m_preset.set(m_selectedActiveByName, uniform.name, v);
    });
}

void vkShade::EffectsPanel::render_uniform_input(const Uniform& uniform)
{
    ImGui::TextColored(UIStyle::Palette::YELLOW, "Unsupported widget: Input");
}

void vkShade::EffectsPanel::render_uniform_radio(const Uniform& uniform)
{
    ImGui::TextColored(UIStyle::Palette::YELLOW, "Unsupported widget: Radio");
}

void vkShade::EffectsPanel::render_uniform_slider(const Uniform& uniform)
{
    Uniform::dispatch_type(uniform.baseType, uniform.components,
        [&]<typename T>(std::type_identity<T>)
    {
        auto value = m_preset.get<T>(m_selectedActiveByName, uniform.name);
        if (!value)
        {
            ImGui::TextColored(UIStyle::Palette::RED, "Invalid uniform value");
            return;
        }

        T v = *value;

        ImGuiDataType dataType;
        Uniform::Scalar min;
        Uniform::Scalar max;
        std::string format;

        switch (uniform.baseType)
        {
            case Uniform::BaseType::Float:
                dataType = ImGuiDataType_Float;
                min = get_ui_value(uniform.uiMin, 0.0f);
                max = get_ui_value(uniform.uiMax, 1.0f);
                format = "%.3f" + uniform.uiUnits;
                break;

            case Uniform::BaseType::Int:
                dataType = ImGuiDataType_S32;
                min = get_ui_value(uniform.uiMin, int32_t{0});
                max = get_ui_value(uniform.uiMax, int32_t{100});
                format = "%d" + uniform.uiUnits;
                break;

            case Uniform::BaseType::Uint:
                dataType = ImGuiDataType_U32;
                min = get_ui_value(uniform.uiMin, uint32_t{0});
                max = get_ui_value(uniform.uiMax, uint32_t{100});
                format = "%u" + uniform.uiUnits;
                break;

            default:
                ImGui::TextColored(UIStyle::Palette::RED, "Invalid uniform type for slider");
                return;
        }

        if (ImGui::SliderScalarN("##value", dataType, &v, uniform.components, &min, &max, format.c_str()))
            m_preset.set(m_selectedActiveByName, uniform.name, v);
    });
}

void vkShade::EffectsPanel::render_uniform_step_control(const Uniform& uniform)
{
    ImGui::BeginGroup();

    if (ImGui::Button("-"))
        step_component(uniform, -1);

    ImGui::SameLine();

    if (ImGui::Button("+"))
        step_component(uniform, 1);

    ImGui::EndGroup();
}

void vkShade::EffectsPanel::step_component(const Uniform& uniform, int32_t direction)
{
    Uniform::dispatch_type(uniform.baseType, uniform.components, [&]<typename T>(std::type_identity<T>)
    {
        using Scalar = typename UniformTraits<T>::Scalar;

        if constexpr (UniformTraits<T>::components == 1 && (std::is_same_v<Scalar, float> ||
            std::is_same_v<Scalar, int32_t> || std::is_same_v<Scalar, uint32_t>))
        {
            auto value = m_preset.get<T>(m_selectedActiveByName, uniform.name);

            if (!value)
                return;

            Scalar& v = *value;

            const Scalar step = get_ui_value(uniform.uiStep, Scalar{1});
            v += step * direction;

            // Clamp between min and max
            if (uniform.uiMin)
                v = std::max(v, get_ui_value(uniform.uiMin, std::numeric_limits<Scalar>::lowest()));
            if (uniform.uiMax)
                v = std::min(v, get_ui_value(uniform.uiMax, std::numeric_limits<Scalar>::max()));

            m_preset.set(m_selectedActiveByName, uniform.name, *value);
        }
    });
}
