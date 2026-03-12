#pragma once

#include <imgui.h>

namespace UI
{
    inline bool Button(const char* label, ImVec2 size, bool enabled, const char* tooltip = nullptr)
    {
        if (!enabled)
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);

        bool clicked = ImGui::Button(label, size) && enabled;

        if (!enabled)
            ImGui::PopStyleVar();

        if (tooltip && ImGui::IsItemHovered() && enabled)
            ImGui::SetTooltip("%s", tooltip);

        return clicked;
    }

    inline bool Button(const char* label, bool enabled, const char* tooltip = nullptr)
    {
        return Button(label, ImVec2(0, 0), enabled, tooltip);
    }

} // namespace UI
