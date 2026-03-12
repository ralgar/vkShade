#pragma once

#include <imgui.h>

namespace vkShade
{
    // Centralized style configuration for consistent UI appearance
    namespace UIStyle
    {
        // Padding and spacing
        constexpr float WINDOW_PADDING = 12.0f;
        constexpr float ITEM_SPACING_X = 8.0f;
        constexpr float ITEM_SPACING_Y = 6.0f;
        constexpr float FRAME_PADDING_X = 8.0f;
        constexpr float FRAME_PADDING_Y = 4.0f;
        constexpr float INDENT_SPACING = 16.0f;

        // Component sizes
        constexpr float BUTTON_HEIGHT = 24.0f;
        constexpr float LISTBOX_MIN_WIDTH = 200.0f;
        constexpr float LISTBOX_MIN_HEIGHT = 300.0f;

        // Colors (ImVec4 in RGBA format, 0.0-1.0)
        constexpr ImVec4 ACCENT_COLOR = ImVec4(0.26f, 0.59f, 0.98f, 1.0f);
        constexpr ImVec4 ACCENT_HOVER = ImVec4(0.36f, 0.69f, 1.0f, 1.0f);
        constexpr ImVec4 ACCENT_ACTIVE = ImVec4(0.16f, 0.49f, 0.88f, 1.0f);
        constexpr ImVec4 BACKGROUND_COLOR = ImVec4(0.12f, 0.12f, 0.13f, 1.0f);
        constexpr ImVec4 FRAME_BG_COLOR = ImVec4(0.20f, 0.20f, 0.22f, 1.0f);

        // Apply style to ImGui context
        inline void ApplyStyle()
        {
            ImGuiStyle& style = ImGui::GetStyle();

            // Padding and spacing
            style.WindowPadding = ImVec2(WINDOW_PADDING, WINDOW_PADDING);
            style.ItemSpacing = ImVec2(ITEM_SPACING_X, ITEM_SPACING_Y);
            style.FramePadding = ImVec2(FRAME_PADDING_X, FRAME_PADDING_Y);
            style.IndentSpacing = INDENT_SPACING;

            // Rounding
            style.WindowRounding = 6.0f;
            style.FrameRounding = 4.0f;
            style.GrabRounding = 3.0f;

            // Colors
            ImVec4* colors = style.Colors;
            colors[ImGuiCol_WindowBg] = BACKGROUND_COLOR;
            colors[ImGuiCol_FrameBg] = FRAME_BG_COLOR;
            colors[ImGuiCol_FrameBgHovered] = ImVec4(0.25f, 0.25f, 0.27f, 1.0f);
            colors[ImGuiCol_FrameBgActive] = ImVec4(0.30f, 0.30f, 0.32f, 1.0f);
            colors[ImGuiCol_Button] = ACCENT_COLOR;
            colors[ImGuiCol_ButtonHovered] = ACCENT_HOVER;
            colors[ImGuiCol_ButtonActive] = ACCENT_ACTIVE;
            colors[ImGuiCol_Header]        = ImVec4(0.26f, 0.59f, 0.98f, 0.4f);
            colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.6f);
            colors[ImGuiCol_HeaderActive]  = ImVec4(0.26f, 0.59f, 0.98f, 0.8f);
        }
    } // namespace UIStyle
} // namespace vkShade
