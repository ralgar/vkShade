#pragma once

#include <expected>
#include <filesystem>
#include <functional>
#include <string>
#include <unordered_map>

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

        std::expected<std::string, Error> get(const std::string& section, const std::string& key) const;
        void set(const std::string& section, const std::string& key, const std::string& value);

    private:
        std::unordered_map<std::string, std::string> m_config;

        void load_config_file(const std::filesystem::path& filePath);
    };
} // namespace vkShade
