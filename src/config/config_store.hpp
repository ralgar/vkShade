#pragma once

#include <expected>
#include <filesystem>
#include <string>
#include <unordered_map>

#include "config_observer.hpp"
#include "config_parser.hpp"
#include "config_types.hpp"

namespace vkShade
{
    class ConfigStore
    {
    public:
        ConfigStore() = default;  // In-memory only

        // Last path is the save target, all paths loaded in order
        ConfigStore(std::vector<std::filesystem::path> filePaths);

        void load();
        void save() const;

        // Typed getter
        template<typename T>
        std::expected<T, ConfigError> get(const std::string& section, const std::string& key) const
        {
            std::string mapKey = section + "::" + key;
            auto it = m_config.find(mapKey);
            if (it == m_config.end())
                return std::unexpected(ConfigError::KeyNotFound);

            return m_parser.parse<T>(it->second);
        }

        // Typed setter
        template<typename T>
        void set(const std::string& section, const std::string& key, const T& value)
        {
            // Normalize pointer types to value types before parsing/notification.
            // We cannot parse to pointer types (e.g., char*) as they would be
            // dangling pointers. String literals (char[N]) decay to char*, so
            // we normalize them to std::string for safe observer notification.
            using DecayedT = std::decay_t<T>;
            using NormalizedT = std::conditional_t<
                std::is_same_v<DecayedT, const char*> || std::is_same_v<DecayedT, char*>,
                std::string,
                DecayedT
            >;

            std::string mapKey = section + "::" + key;
            std::string newValue = m_parser.to_string(value);
            std::string oldValue;

            auto it = m_config.find(mapKey);
            if (it != m_config.end())
                oldValue = it->second;

            if (oldValue != newValue)
            {
                m_config[mapKey] = newValue;

                auto parsed = m_parser.parse<NormalizedT>(newValue);
                if (parsed)
                    m_observer.notify(section, key, *parsed);
            }
        }

        ConfigObserver::Sink on_changed(const std::string& section, const std::string& key)
        {
            return m_observer.on_changed(section, key);
        }

    private:
        ConfigParser   m_parser;
        ConfigObserver m_observer;

        std::vector<std::filesystem::path> m_filePaths;
        std::unordered_map<std::string, std::string> m_config;
    };
} // namespace vkShade
