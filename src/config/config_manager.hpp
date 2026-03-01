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
            else if constexpr (std::is_same_v<DecayedT, glm::vec2>)
                return parse_vec2(str);
            else if constexpr (std::is_same_v<DecayedT, glm::vec3>)
                return parse_vec3(str);
            else if constexpr (std::is_same_v<DecayedT, glm::vec4>)
                return parse_vec4(str);
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

        std::expected<glm::vec2, Error> parse_vec2(const std::string& str) const
        {
            auto components = this->split_list(str);
            if (components.size() != 2)
                return std::unexpected(Error::ParseError);

            auto x = parse_float<float>(components[0]);
            auto y = parse_float<float>(components[1]);

            if (!x || !y)
                return std::unexpected(Error::ParseError);

            return glm::vec2(*x, *y);
        }

        std::expected<glm::vec3, Error> parse_vec3(const std::string& str) const
        {
            auto components = this->split_list(str);
            if (components.size() != 3)
                return std::unexpected(Error::ParseError);

            auto x = parse_float<float>(components[0]);
            auto y = parse_float<float>(components[1]);
            auto z = parse_float<float>(components[2]);

            if (!x || !y || !z)
                return std::unexpected(Error::ParseError);

            return glm::vec3(*x, *y, *z);
        }

        std::expected<glm::vec4, Error> parse_vec4(const std::string& str) const
        {
            auto components = this->split_list(str);
            if (components.size() != 4)
                return std::unexpected(Error::ParseError);

            auto x = parse_float<float>(components[0]);
            auto y = parse_float<float>(components[1]);
            auto z = parse_float<float>(components[2]);
            auto w = parse_float<float>(components[3]);

            if (!x || !y || !z || !w)
                return std::unexpected(Error::ParseError);

            return glm::vec4(*x, *y, *z, *w);
        }

        std::vector<std::string> split_list(const std::string& str) const
        {
            std::vector<std::string> items;
            size_t start = 0;                // Start of current item
            size_t end = str.find(',');      // Find first comma

            // Loop through all commas except the last item
            while (end != std::string::npos)
            {
                std::string item = str.substr(start, end - start);  // Extract item
                item.erase(0, item.find_first_not_of(" \t"));       // Trim leading whitespace
                item.erase(item.find_last_not_of(" \t") + 1);       // Trim trailing whitespace

                if (!item.empty())
                    items.push_back(item);

                start = end + 1;              // Move past the comma
                end = str.find(',', start);   // Find next comma
            }

            // Manually handle the last item (no comma after it)
            std::string item = str.substr(start);
            item.erase(0, item.find_first_not_of(" \t"));
            item.erase(item.find_last_not_of(" \t") + 1);

            if (!item.empty())
                items.push_back(item);

            return items;
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
            else if constexpr (std::is_same_v<DecayedT, glm::vec2>)
                return std::to_string(value.x) + "," + std::to_string(value.y);
            else if constexpr (std::is_same_v<DecayedT, glm::vec3>)
                return std::to_string(value.x) + "," + std::to_string(value.y) + "," + std::to_string(value.z);
            else if constexpr (std::is_same_v<DecayedT, glm::vec4>)
                return std::to_string(value.x) + "," + std::to_string(value.y) + "," +
                       std::to_string(value.z) + "," + std::to_string(value.w);
            else
                return std::to_string(value);
        }
    };
} // namespace vkShade
