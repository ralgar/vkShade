#include "config_manager.hpp"

#include <ini.h>
#include <spdlog/spdlog.h>

// Static callback function for inih parser
static int config_ini_handler(void* user, const char* section, const char* name, const char* value)
{
    auto* config = static_cast<std::unordered_map<std::string, std::string>*>(user);

    // Create flat, namespaced key like "Section::Name"
    std::string key = std::string(section) + "::" + std::string(name);
    (*config)[key] = value;

    spdlog::trace("Parsed configuration option: {}::{} ({})", section, name, value);

    return 1;  // Success
}

vkShade::ConfigManager::ConfigManager()
{
    // FIXME: Load from standard paths instead of hardcoded test path
    const std::filesystem::path path = "./config/preset.ini";

    load_config_file(path);
}

std::expected<std::string, vkShade::ConfigManager::Error>
vkShade::ConfigManager::get(const std::string& section, const std::string& key) const
{
    std::string mapKey = section + "::" + key;

    auto it = m_config.find(mapKey);
    if (it == m_config.end())
        return std::unexpected(Error::SectionNotFound);

    return it->second;
}

void vkShade::ConfigManager::load_config_file(const std::filesystem::path& filePath)
{
    if (!std::filesystem::exists(filePath))
        return; // Silently skip non-existent files

    // Parse the INI file
    int result = ini_parse(filePath.string().c_str(), config_ini_handler, &m_config);
    if (result != 0)
        spdlog::error("Failed to load or parse config file: {}", filePath.c_str());
}

void vkShade::ConfigManager::set(const std::string& section, const std::string& key, const std::string& value)
{
    std::string mapKey = section + "::" + key;
    m_config[mapKey] = value;
}
