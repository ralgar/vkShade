#include "log_panel.hpp"

#include "core/logger.hpp"
#include "gui/gui_style.hpp"
#include "imgui.h"

std::string vkShade::LogPanel::format_log_line(const LogMessage& message)
{
    const std::string timestamp = format_timestamp(message.time);

    const char* level = level_text(message.level);

    std::string result;

    result.reserve(timestamp.size() + 10 + message.payload.size());

    result += timestamp;
    result += level;
    result.append(message.payload.data(), message.payload.size());

    return result;
}

std::string vkShade::LogPanel::format_timestamp(const std::chrono::system_clock::time_point& time)
{
    using namespace std::chrono;

    const auto time_t = system_clock::to_time_t(time);

    std::tm tm{};
    localtime_r(&time_t, &tm);

    const auto milliseconds =
        duration_cast<std::chrono::milliseconds>(time.time_since_epoch()) % 1000;

    char timeBuffer[32];

    std::strftime(timeBuffer, sizeof(timeBuffer), "[%H:%M:%S", &tm);

    char buffer[64];

    std::snprintf(buffer, sizeof(buffer),
        "%s.%03lld]",
        timeBuffer,
        static_cast<long long>(milliseconds.count()));

    return buffer;
}

const char* vkShade::LogPanel::level_text(spdlog::level::level_enum level)
{
    switch (level)
    {
        case spdlog::level::trace:    return "[trace]";
        case spdlog::level::debug:    return "[debug]";
        case spdlog::level::info:     return "[info] ";
        case spdlog::level::warn:     return "[warn] ";
        case spdlog::level::err:      return "[error]";
        case spdlog::level::critical: return "[crit] ";
        default:                      return "[?????]";
    }
}

ImVec4 vkShade::LogPanel::level_color(spdlog::level::level_enum level)
{
    switch (level)
    {
        case spdlog::level::trace:
            return UIStyle::Palette::WHITE;

        case spdlog::level::debug:
            return UIStyle::Palette::BLUE;

        case spdlog::level::info:
            return UIStyle::Palette::GREEN;

        case spdlog::level::warn:
            return UIStyle::Palette::YELLOW;

        case spdlog::level::err:
            return UIStyle::Palette::RED;

        case spdlog::level::critical:
            return UIStyle::Palette::RED;

        default:
            return ImGui::GetStyle().Colors[ImGuiCol_Text];
    }
}

bool vkShade::LogPanel::passes_filter(const LogMessage& message) const
{
    if ((m_logLevelMask & (1u << message.level)) == 0)
        return false;

    if (m_filterBuffer[0] != '\0')
    {
        const std::string_view filter(m_filterBuffer);

        const std::string_view payload(message.payload.data(), message.payload.size());

        if (payload.find(filter) == std::string_view::npos)
            return false;
    }

    return true;
}

void vkShade::LogPanel::render()
{
    render_control_bar();

    ImGui::Separator();

    render_log_messages();
}

void vkShade::LogPanel::render_control_bar()
{
    // Increase horizontal spacing between controls
    ImVec2 spacing = ImGui::GetStyle().ItemSpacing;
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(spacing.x * 2.0f, spacing.y));

    // TODO: Finish this after clipboard is supported
    /*if (ImGui::Button("Copy"))
    {
        std::string text;

        for (const auto& message : logHistory)
        {
            // Only copy what passes the filter
            //if (!passes_filter(message))
            //    continue;

            text += format_log_line(message);
            text += '\n';
        }

        ImGui::SetClipboardText(text.c_str());
    }

    ImGui::SameLine();
    */

    ImGui::Checkbox("Follow", &m_logFollow);

    ImGui::SameLine();

    // Level filter dropdown
    {
        std::string levelSummary;

        const auto addLevelToSummary = [&](const char* name, spdlog::level::level_enum level)
        {
            if ((m_logLevelMask & (1u << level)) != 0)
            {
                if (!levelSummary.empty())
                    levelSummary += ", ";

                levelSummary += name;
            }
        };

        addLevelToSummary("Error", spdlog::level::err);
        addLevelToSummary("Warn",  spdlog::level::warn);
        addLevelToSummary("Info",  spdlog::level::info);
        addLevelToSummary("Debug", spdlog::level::debug);
        addLevelToSummary("Trace", spdlog::level::trace);

        if (levelSummary.empty())
            levelSummary = "None";

        // Calculate maximum width for combo box
        const char* levels[] = {"Trace", "Debug", "Info", "Warn", "Error"};

        std::string maxSummary {" "};
        for (const char* level : levels)
        {
            if (!maxSummary.empty())
                maxSummary += ", ";

            maxSummary += level;
        }

        float comboWidth = ImGui::CalcTextSize(maxSummary.c_str()).x +
                           ImGui::GetStyle().FramePadding.x * 2.0f +
                           ImGui::GetStyle().ScrollbarSize;

        ImGui::SetNextItemWidth(comboWidth);

        const float comboHeight = ImGui::GetFrameHeightWithSpacing() * 8.0f;

        ImGui::SetNextWindowSizeConstraints(ImVec2(0.0f, 0.0f), ImVec2(FLT_MAX, comboHeight));

        if (ImGui::BeginCombo("##Levels", levelSummary.c_str()))
        {
            const auto toggleLevel = [&](const char* label, spdlog::level::level_enum level)
            {
                bool enabled = (m_logLevelMask & (1u << level)) != 0;

                if (ImGui::Checkbox(label, &enabled))
                {
                    if (enabled)
                        m_logLevelMask |= (1u << level);
                    else
                        m_logLevelMask &= ~(1u << level);
                }
            };

            toggleLevel("Error",    spdlog::level::err);
            toggleLevel("Warn",     spdlog::level::warn);
            toggleLevel("Info",     spdlog::level::info);
            toggleLevel("Debug",    spdlog::level::debug);
            toggleLevel("Trace",    spdlog::level::trace);

            ImGui::Separator();

            if (ImGui::Button("All"))
            {
                m_logLevelMask = (1u << spdlog::level::trace) |
                                 (1u << spdlog::level::debug) |
                                 (1u << spdlog::level::info) |
                                 (1u << spdlog::level::warn) |
                                 (1u << spdlog::level::err);
            }

            ImGui::SameLine();

            if (ImGui::Button("None"))
                m_logLevelMask = 0;

            ImGui::EndCombo();
        }
    } // Level filter dropdown

    ImGui::SameLine();

    // Text filter
    {
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

        ImGui::InputTextWithHint("##LogFilter", "Filter...",
            m_filterBuffer, sizeof(m_filterBuffer));

        ImGui::PopStyleVar();
    }
}

void vkShade::LogPanel::render_log_messages()
{
    const auto logHistory = Logger::get_history();

    ImGui::PushStyleColor(ImGuiCol_ChildBg, UIStyle::Palette::BACKGROUND);

    if (ImGui::BeginChild("LogMessages", ImVec2(0, 0),false,
        ImGuiWindowFlags_HorizontalScrollbar))
    {
        for (const auto& message : logHistory)
        {
            if (!passes_filter(message))
                continue;

            // Timestamp
            const std::string timestamp = format_timestamp(message.time);
            ImGui::TextDisabled("%s", timestamp.c_str());

            ImGui::SameLine();

            // Level
            ImGui::PushStyleColor(ImGuiCol_Text, level_color(message.level));

            ImGui::Text("%s", level_text(message.level));

            ImGui::PopStyleColor();

            ImGui::SameLine();

            // Message
            ImGui::PushStyleColor(ImGuiCol_Text, UIStyle::Palette::FOREGROUND);

            ImGui::TextUnformatted(message.payload.data(),
                message.payload.data() + message.payload.size());

            ImGui::PopStyleColor();
        }

        // Follow
        if (m_logFollow)
            ImGui::SetScrollY(ImGui::GetScrollMaxY());
    }

    ImGui::EndChild();

    ImGui::PopStyleColor();
}
