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
        enum class Type
        {
            Config,
            Internal,
            Preset,
        };

        explicit ConfigStore(Type type);

        void clear();
        bool load(std::filesystem::path filePath);
        bool save(std::filesystem::path filePath);  // Save As
        bool save();  // Save currently open file

        bool has_file() const { return !m_currentFile.empty(); }
        std::filesystem::path get_path() const { return m_currentFile; }

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
        Type m_type;
        std::string m_typeString;

        ConfigParser   m_parser;
        ConfigObserver m_observer;

        std::filesystem::path m_currentFile;
        std::unordered_map<std::string, std::string> m_config;
    };
} // namespace vkShade

#include "config_observer.inl"
