#pragma once
#include "panel.hpp"

#include <cstdint>

#include <spdlog/spdlog.h>
#include <imgui.h>

namespace vkShade
{
    // Main GUI window
    class LogPanel : public GuiPanel
    {
    public:
        LogPanel() = default;

        void render() override;

    private:
        using LogMessage = spdlog::details::log_msg_buffer;

        bool m_logFollow = true;

        uint32_t m_logLevelMask = (1u << spdlog::level::info) |
                                  (1u << spdlog::level::warn) |
                                  (1u << spdlog::level::err);

        char m_filterBuffer[256] {};

        void render_control_bar();
        void render_log_messages();
        void render_level_control();

        bool passes_filter(const LogMessage& message) const;

        static const char* level_text(spdlog::level::level_enum level);
        static ImVec4 level_color(spdlog::level::level_enum level);

        static std::string format_log_line(const LogMessage& message);
        static std::string format_timestamp(const std::chrono::system_clock::time_point& time);
    };
} // namespace vkShade
