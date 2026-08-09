#include "log_panel.hpp"

#include <imgui.h>

#include "core/logger.hpp"

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
                ImGui::TextUnformatted(begin, end);
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
