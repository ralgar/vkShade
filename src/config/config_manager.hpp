#pragma once

#include <expected>
#include <filesystem>
#include <string>
#include <unordered_map>

#include "core/type_traits.hpp"

namespace vkShade
{
    class ConfigManager
    {
    public:
        ConfigManager();

        enum class Error
        {
            KeyNotFound,
            SectionNotFound,
            FileNotFound,
            ParseError
        };

        // Typed getter
        template<typename T>
        std::expected<T, Error> get(const std::string& section, const std::string& key) const
        {
            std::string mapKey = section + "::" + key;
            auto it = m_config.find(mapKey);
            if (it == m_config.end())
                return std::unexpected(Error::KeyNotFound);

            return parse<T>(it->second);
        }

        // Typed setter
        template<typename T>
        void set(const std::string& section, const std::string& key, const T& value)
        {
            std::string mapKey = section + "::" + key;
            std::string newValue = this->to_string(value);
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
        std::unordered_map<std::string, std::string> m_config;

        void load_config_file(const std::filesystem::path& filePath);

        // Parse string value to typed value
        template<typename T>
        std::expected<T, Error> parse(const std::string& str) const
        {
            using DecayedT = std::decay_t<T>;

            if constexpr (std::is_same_v<DecayedT, std::string>)
                return str;
            else if constexpr (std::is_same_v<DecayedT, bool>)
                return parse_bool<DecayedT>(str);
            else if constexpr (std::is_floating_point_v<DecayedT>)
                return parse_float<DecayedT>(str);
            else if constexpr (std::is_integral_v<DecayedT>)
                return parse_integral<DecayedT>(str);
            else
                static_assert(always_false<T>::value, "Unsupported type for config parsing");
        }

        std::expected<bool, Error> parse_bool(const std::string& str) const
        {
            std::string lower = str;
            std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

            if (lower == "true" || lower == "1" || lower == "yes" || lower == "on")
                return true;
            if (lower == "false" || lower == "0" || lower == "no" || lower == "off")
                return false;

            return std::unexpected(Error::ParseError);
        }

        template<typename T>
        std::expected<T, Error> parse_float(const std::string& str) const
        {
            try {
                if constexpr (std::is_same_v<T, float>)
                    return std::stof(str);
                else
                    return std::stod(str);
            } catch (...) {
                return std::unexpected(Error::ParseError);
            }
        }

        template<typename T>
        std::expected<T, Error> parse_integral(const std::string& str) const
        {
            try {
                if constexpr (std::is_signed_v<T>)
                    return static_cast<T>(std::stoll(str));
                else
                    return static_cast<T>(std::stoull(str));
            } catch (...) {
                return std::unexpected(Error::ParseError);
            }
        }

        // Handle string conversions not supported by std::to_string
        template<typename T>
        std::string to_string(const T& value) const
        {
            // Strip references, const qualifiers, and decay arrays to pointers for type checking.
            // Needed for passing string literals, but also improves robustness in general.
            using DecayedT = std::decay_t<T>;

            if constexpr (std::is_same_v<DecayedT, std::string>)
                return value;
            else if constexpr (std::is_same_v<DecayedT, const char*> || std::is_same_v<DecayedT, char*>)
                return std::string(value);
            else if constexpr (std::is_same_v<DecayedT, bool>)
                return value ? "true" : "false";
            else
                return std::to_string(value);
        }
    };
} // namespace vkShade
