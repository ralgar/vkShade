#pragma once

#include <algorithm>
#include <expected>
#include <string>
#include <sstream>
#include <vector>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include "core/type_traits.hpp"
#include "config_types.hpp"

namespace vkShade
{
    class ConfigParser
    {
    public:
        // Parse string value to typed value
        template<typename T>
        std::expected<T, ConfigError> parse(const std::string& str) const
        {
            // Strip references, const qualifiers, and decay arrays to pointers for type checking.
            // Needed for passing string literals, but also improves robustness in general.
            using DecayedT = std::decay_t<T>;

            // Note: We cannot safely parse to pointer types (e.g., char*) because
            // we would return a pointer to a temporary string's buffer, creating
            // a dangling pointer. Callers must normalize pointer types to value
            // types (e.g., char* -> std::string) before calling parse().
            static_assert(!std::is_pointer_v<DecayedT>, "Cannot parse to pointer types");

            if constexpr (std::is_same_v<DecayedT, std::string>)
                return str;
            else if constexpr (std::is_same_v<DecayedT, std::vector<std::string>>)
                return split_list(str);
            else if constexpr (std::is_same_v<DecayedT, bool>)
                return parse_bool(str);
            else if constexpr (std::is_same_v<DecayedT, float>)
                return parse_float(str);
            else if constexpr (std::is_integral_v<DecayedT>)
                return parse_integer<DecayedT>(str);
            else if constexpr (std::is_same_v<DecayedT, glm::vec2>)
                return parse_vec2(str);
            else if constexpr (std::is_same_v<DecayedT, glm::vec3>)
                return parse_vec3(str);
            else if constexpr (std::is_same_v<DecayedT, glm::vec4>)
                return parse_vec4(str);
            else
                static_assert(always_false<T>::value, "Unsupported type for config parsing");
        }

        // Handle string conversions not supported by std::to_string
        template<typename T>
        std::string to_string(const T& value) const
        {
            using DecayedT = std::decay_t<T>;

            if constexpr (std::is_same_v<DecayedT, std::string>)
                return value;
            else if constexpr (std::is_same_v<DecayedT, const char*> || std::is_same_v<DecayedT, char*>)
                return std::string(value);
            else if constexpr (std::is_same_v<DecayedT, std::vector<std::string>>)
                return join_list(value);
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

    private:
        std::string join_list(const std::vector<std::string>& items) const
        {
            if (items.empty()) return "";

            std::string result = items[0];
            for (size_t i = 1; i < items.size(); i++)
            {
                result += "," + items[i];
            }

            return result;
        }

        std::expected<bool, ConfigError> parse_bool(const std::string& str) const
        {
            std::string lower = str;
            std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

            if (lower == "true" || lower == "1" || lower == "yes" || lower == "on")
                return true;
            if (lower == "false" || lower == "0" || lower == "no" || lower == "off")
                return false;

            return std::unexpected(ConfigError::ParseError);
        }

        std::expected<float, ConfigError> parse_float(const std::string& str) const
        {
            try {
                return std::stof(str);
            } catch (...) {
                return std::unexpected(ConfigError::ParseError);
            }
        }

        template<typename T>
        std::expected<T, ConfigError> parse_integer(const std::string& str) const
        {
            static_assert(std::is_integral_v<T>);

            try {
                if constexpr (std::is_signed_v<T>)
                {
                    int64_t value = std::stoll(str);
                    if (value < std::numeric_limits<T>::min() || value > std::numeric_limits<T>::max())
                        return std::unexpected(ConfigError::ParseError);
                    return static_cast<T>(value);
                }
                else
                {
                    uint64_t value = std::stoull(str);
                    if (value > std::numeric_limits<T>::max())
                        return std::unexpected(ConfigError::ParseError);
                    return static_cast<T>(value);
                }
            } catch (...) {
                return std::unexpected(ConfigError::ParseError);
            }
        }

        std::expected<glm::vec2, ConfigError> parse_vec2(const std::string& str) const
        {
            auto components = this->split_list(str);
            if (components.size() != 2)
                return std::unexpected(ConfigError::ParseError);

            auto x = parse_float(components[0]);
            auto y = parse_float(components[1]);

            if (!x || !y)
                return std::unexpected(ConfigError::ParseError);

            return glm::vec2(*x, *y);
        }

        std::expected<glm::vec3, ConfigError> parse_vec3(const std::string& str) const
        {
            auto components = this->split_list(str);
            if (components.size() != 3)
                return std::unexpected(ConfigError::ParseError);

            auto x = parse_float(components[0]);
            auto y = parse_float(components[1]);
            auto z = parse_float(components[2]);

            if (!x || !y || !z)
                return std::unexpected(ConfigError::ParseError);

            return glm::vec3(*x, *y, *z);
        }

        std::expected<glm::vec4, ConfigError> parse_vec4(const std::string& str) const
        {
            auto components = this->split_list(str);
            if (components.size() != 4)
                return std::unexpected(ConfigError::ParseError);

            auto x = parse_float(components[0]);
            auto y = parse_float(components[1]);
            auto z = parse_float(components[2]);
            auto w = parse_float(components[3]);

            if (!x || !y || !z || !w)
                return std::unexpected(ConfigError::ParseError);

            return glm::vec4(*x, *y, *z, *w);
        }

        std::vector<std::string> split_list(const std::string& str) const
        {
            auto trim = [](std::string s)
            {
                s.erase(0, s.find_first_not_of(" \t"));
                s.erase(s.find_last_not_of(" \t") + 1);
                return s;
            };

            std::vector<std::string> items;
            std::stringstream ss(str);
            std::string item;

            while (std::getline(ss, item, ','))
            {
                item = trim(item);
                if (!item.empty())
                    items.push_back(item);
            }

            return items;
        }
    };
} // namespace vkShade
