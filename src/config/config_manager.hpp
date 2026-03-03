#pragma once

#include <expected>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include "core/type_traits.hpp"
#include "config_parser.hpp"
#include "config_types.hpp"

namespace vkShade
{
    class ConfigManager
    {
    public:
        ConfigManager();

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
            std::string mapKey = section + "::" + key;
            std::string newValue = m_parser.to_string(value);
            std::string oldValue;

            auto it = m_config.find(mapKey);
            if (it != m_config.end())
                oldValue = it->second;

            if (oldValue != newValue)
            {
                m_config[mapKey] = newValue;
                // TODO: Send event with old/new values
            }
        }

    private:
        ConfigParser m_parser;

        std::unordered_map<std::string, std::string> m_config;

        void load_config_file(const std::filesystem::path& filePath);
    };
} // namespace vkShade
