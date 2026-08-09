#pragma once

#include <cstdlib>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace vkShade
{
    // vkShade deliberately owns this logger instead of using spdlog's global
    // registry. Other Vulkan layers in the same process may replace or clear
    // that global registry at any time.
    class Logger
    {
    public:
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
        static spdlog::logger& get()
        {
            // Keep the logger independent from spdlog's process-wide registry.
            static auto logger = []
            {
                std::vector<spdlog::sink_ptr> sinks;
                sinks.push_back(std::make_shared<spdlog::sinks::stderr_color_sink_mt>());

                if (const char* path = std::getenv("VKSHADE_LOG_FILE"))
                    // Function-local static initialization opens and truncates the log
                    // exactly once, even when accessed concurrently from multiple threads.
                    sinks.push_back(
                        std::make_shared<spdlog::sinks::basic_file_sink_mt>(path, true));

                auto result = std::make_shared<spdlog::logger>(
                    "vkShade", sinks.begin(), sinks.end());
                result->flush_on(spdlog::level::trace);

                const char* configuredLevel = std::getenv("VKSHADE_LOG_LEVEL");
                const std::string level = configuredLevel ? configuredLevel : "info";
                if (level == "trace")
                    result->set_level(spdlog::level::trace);
                else if (level == "debug")
                    result->set_level(spdlog::level::debug);
                else if (level == "info")
                    result->set_level(spdlog::level::info);
                else if (level == "warn" || level == "warning")
                    result->set_level(spdlog::level::warn);
                else if (level == "error")
                    result->set_level(spdlog::level::err);
                else if (level == "critical")
                    result->set_level(spdlog::level::critical);
                else if (level == "off")
                    result->set_level(spdlog::level::off);
                else
                    result->set_level(spdlog::level::info);

                result->debug("vkShade logger initialized");
                return result;
            }();

            return *logger;
        }
    };
}
