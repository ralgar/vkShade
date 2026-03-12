#include "about_window.hpp"

#include <imgui.h>

#include "version.hpp" // cppcheck-suppress missingInclude

void vkShade::AboutWindow::render()
{
    ImGui::SetNextWindowSize(ImVec2(300, 160), ImGuiCond_Always);
    if (ImGui::Begin("About", &m_visible, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse))
    {
        // Center the title
        float windowWidth = ImGui::GetContentRegionAvail().x;
        float textWidth = ImGui::CalcTextSize("vkShade").x;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (windowWidth - textWidth) * 0.5f);
        ImGui::Text("vkShade");

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::TextDisabled("A Vulkan post-processing layer.");

        ImGui::Spacing();

        ImGui::TextDisabled(VKSHADE_VERSION);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Copyright at the bottom
        ImGui::TextDisabled("Copyright (c) 2026 Ryan Algar");
    }
    ImGui::End();
}
