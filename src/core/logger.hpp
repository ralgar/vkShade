#pragma once

#include <cstdlib>
#include <memory>
#include <spdlog/details/log_msg.h>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/ringbuffer_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace vkShade
{
    // vkShade deliberately owns this logger instead of using spdlog's global
    // registry. Other Vulkan layers in the same process may replace or clear
    // that global registry at any time.
    class Logger
    {
    public:
        static std::vector<spdlog::details::log_msg_buffer> get_history()
        {
            return state().history->last_raw(m_historySize);
        }

        static void trace(std::string_view msg)
        {
            get().trace(msg);
        }

        template <typename... Args>
        static void trace(spdlog::format_string_t<Args...> fmt, Args&&... args)
        {
            get().trace(fmt, std::forward<Args>(args)...);
        }

        static void debug(std::string_view msg)
        {
            get().debug(msg);
        }

        template <typename... Args>
        static void debug(spdlog::format_string_t<Args...> fmt, Args&&... args)
        {
            get().debug(fmt, std::forward<Args>(args)...);
        }

        static void info(std::string_view msg)
        {
            get().info(msg);
        }

        template <typename... Args>
        static void info(spdlog::format_string_t<Args...> fmt, Args&&... args)
        {
            get().info(fmt, std::forward<Args>(args)...);
        }

        static void warn(std::string_view msg)
        {
            get().warn(msg);
        }

        template <typename... Args>
        static void warn(spdlog::format_string_t<Args...> fmt, Args&&... args)
        {
            get().warn(fmt, std::forward<Args>(args)...);
        }

        static void error(std::string_view msg)
        {
            get().error(msg);
        }

        template <typename... Args>
        static void error(spdlog::format_string_t<Args...> fmt, Args&&... args)
        {
            get().error(fmt, std::forward<Args>(args)...);
        }

        static void critical(std::string_view msg)
        {
            get().critical(msg);
        }

        template <typename... Args>
        static void critical(spdlog::format_string_t<Args...> fmt, Args&&... args)
        {
            get().critical(fmt, std::forward<Args>(args)...);
        }

    private:
        static constexpr size_t m_historySize = 2000;

        struct State
        {
            std::shared_ptr<spdlog::sinks::ringbuffer_sink_mt> history;
            std::shared_ptr<spdlog::logger> logger;
        };

        static spdlog::logger& get()
        {
            return *state().logger;
        }

        static State& state()
        {
            // Keep the logger independent from spdlog's process-wide registry.
            static State instance = []
            {
                State state;

                std::vector<spdlog::sink_ptr> sinks;

                auto stderrSink = std::make_shared<spdlog::sinks::stderr_color_sink_mt>();
                sinks.push_back(stderrSink);

                state.history = std::make_shared<spdlog::sinks::ringbuffer_sink_mt>(m_historySize);
                state.history->set_level(spdlog::level::trace);
                sinks.push_back(state.history);

                spdlog::sink_ptr fileSink;
                if (const char* path = std::getenv("VKSHADE_LOG_FILE"))
                {
                    // Function-local static initialization opens and truncates the log
                    // exactly once, even when accessed concurrently from multiple threads.
                    fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path, true);
                    sinks.push_back(fileSink);
                }

                state.logger = std::make_shared<spdlog::logger>("vkShade", sinks.begin(), sinks.end());

                const char* configuredLevel = std::getenv("VKSHADE_LOG_LEVEL");
                const std::string level = configuredLevel ? configuredLevel : "info";

                spdlog::level::level_enum configuredLogLevel;

                if (level == "trace")
                    configuredLogLevel = spdlog::level::trace;
                else if (level == "debug")
                    configuredLogLevel = spdlog::level::debug;
                else if (level == "info")
                    configuredLogLevel = spdlog::level::info;
                else if (level == "warn" || level == "warning")
                    configuredLogLevel = spdlog::level::warn;
                else if (level == "error")
                    configuredLogLevel = spdlog::level::err;
                else if (level == "critical")
                    configuredLogLevel = spdlog::level::critical;
                else if (level == "off")
                    configuredLogLevel = spdlog::level::off;
                else
                    configuredLogLevel = spdlog::level::info;

                // The logger must accept everything so the history sink
                //  always receives trace messages.
                state.logger->set_level(spdlog::level::trace);

                // Apply the configured verbosity only to the external sinks.
                stderrSink->set_level(configuredLogLevel);
                state.logger->flush_on(configuredLogLevel);

                if (fileSink)
                    fileSink->set_level(configuredLogLevel);

                state.logger->debug("vkShade logger initialized");
                return state;
            }();

            return instance;
        }
    };
}
