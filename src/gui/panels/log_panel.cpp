#include "log_panel.hpp"

#include <string_view>

#include <imgui.h>

#include "core/logger.hpp"
#include "gui/gui_style.hpp"

namespace
{
    enum class LogMessageLevel
    {
        Unknown,
        Trace,
        Debug,
        Info,
        Warning,
        Error,
        Critical,
    };

    ImVec4 blend_color(const ImVec4& from, const ImVec4& to, float amount)
    {
        return ImVec4(
            from.x + (to.x - from.x) * amount,
            from.y + (to.y - from.y) * amount,
            from.z + (to.z - from.z) * amount,
            from.w + (to.w - from.w) * amount);
    }

    LogMessageLevel log_message_level(std::string_view message)
    {
        const size_t loggerSeparator = message.find("] [");
        if (loggerSeparator == std::string_view::npos)
            return LogMessageLevel::Unknown;

        const size_t levelSeparator = message.find("] [", loggerSeparator + 3);
        if (levelSeparator == std::string_view::npos)
            return LogMessageLevel::Unknown;

        const size_t levelBegin = levelSeparator + 3;
        const size_t levelEnd = message.find(']', levelBegin);
        if (levelEnd == std::string_view::npos)
            return LogMessageLevel::Unknown;

        const std::string_view level = message.substr(levelBegin, levelEnd - levelBegin);
        if (level == "trace")
            return LogMessageLevel::Trace;
        if (level == "debug")
            return LogMessageLevel::Debug;
        if (level == "info")
            return LogMessageLevel::Info;
        if (level == "warning")
            return LogMessageLevel::Warning;
        if (level == "error")
            return LogMessageLevel::Error;
        if (level == "critical")
            return LogMessageLevel::Critical;
        return LogMessageLevel::Unknown;
    }
}

void vkShade::LogPanel::render()
{
    if (!m_paused &&
        (m_messages.empty() || ImGui::GetTime() >= m_nextRefresh))
    {
        m_messages = Logger::recent_messages(Logger::recent_message_capacity);
        m_nextRefresh = ImGui::GetTime() + 0.25;
    }

    ImGui::Checkbox("Pause", &m_paused);
    ImGui::SameLine();
    ImGui::Checkbox("Follow", &m_follow);
    ImGui::SameLine();
    ImGui::TextDisabled("%zu / %zu messages", m_messages.size(),
                        Logger::recent_message_capacity);

    ImGui::Separator();

    if (ImGui::BeginChild("LogMessages", ImVec2(0, 0), false,
                          ImGuiWindowFlags_HorizontalScrollbar))
    {
        ImGuiListClipper clipper;
        clipper.Begin(static_cast<int>(m_messages.size()));
        while (clipper.Step())
        {
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i)
            {
                const std::string& message = m_messages[i];
                const char* begin = message.data();
                const char* end = begin + message.size();
                while (end != begin && (end[-1] == '\n' || end[-1] == '\r'))
                    --end;

                const ImVec4 textColor = ImGui::GetStyleColorVec4(ImGuiCol_Text);
                const ImVec4 disabledColor =
                    ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled);
                ImVec4 levelColor = textColor;
                bool useLevelColor = true;

                switch (log_message_level(std::string_view(begin, end)))
                {
                    case LogMessageLevel::Trace:
                        levelColor = disabledColor;
                        break;
                    case LogMessageLevel::Debug:
                        levelColor = blend_color(disabledColor, textColor, 0.35f);
                        break;
                    case LogMessageLevel::Warning:
                        levelColor = blend_color(
                            textColor, UIStyle::Palette::YELLOW, 0.65f);
                        break;
                    case LogMessageLevel::Error:
                        levelColor = blend_color(
                            textColor, UIStyle::Palette::RED, 0.70f);
                        break;
                    case LogMessageLevel::Critical:
                        levelColor = blend_color(
                            textColor, UIStyle::Palette::RED, 0.85f);
                        break;
                    case LogMessageLevel::Info:
                    case LogMessageLevel::Unknown:
                        useLevelColor = false;
                        break;
                }

                if (useLevelColor)
                    ImGui::PushStyleColor(ImGuiCol_Text, levelColor);
                ImGui::TextUnformatted(begin, end);
                if (useLevelColor)
                    ImGui::PopStyleColor();
            }
        }

        if (m_follow)
        {
            ImGui::Dummy(ImVec2(0, 0));
            ImGui::SetScrollHereY(1.0f);
        }
    }
    ImGui::EndChild();
}
