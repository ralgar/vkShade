#include "config_store.hpp"

#include <fstream>
#include <map>

#include <ini.h>
#include <magic_enum/magic_enum.hpp>

#include "core/logger.hpp"

// Static callback function for inih parser
static int config_ini_handler(void* user, const char* section, const char* name, const char* value)
{
    auto* config = static_cast<std::unordered_map<std::string, std::string>*>(user);

    // Create flat, namespaced key like "Section::Name"
    std::string key = std::string(section) + "::" + std::string(name);
    (*config)[key] = value;

    vkShade::Logger::trace("Parsed configuration option: {}::{} ({})", section, name, value);

    return 1;  // Success
}

vkShade::ConfigStore::ConfigStore(Type type)
    : m_type(type)
{
    m_typeString = std::string(magic_enum::enum_name(m_type));
    std::transform(m_typeString.begin(), m_typeString.end(), m_typeString.begin(), ::tolower);
}

void vkShade::ConfigStore::clear()
{
    m_observer.notify_all_defaults();
    m_currentFile = std::filesystem::path{};
    m_config.clear();
}

bool vkShade::ConfigStore::load(std::filesystem::path filePath)
{
    if (!std::filesystem::exists(filePath))
        return false;

    if (!std::filesystem::is_regular_file(filePath))
        return false;

    auto perms = std::filesystem::status(filePath).permissions();
    if ((perms & std::filesystem::perms::owner_read) == std::filesystem::perms::none)
        return false;

    // Parse the INI file
    int result = ini_parse(filePath.string().c_str(), config_ini_handler, &m_config);
    if (result != 0)
    {
        Logger::error("Failed to load or parse config file: {}", filePath.c_str());
        return false;
    }

    Logger::info("Loaded {} file: {}", m_typeString, filePath.string());

    m_currentFile = filePath;
    return true;
}

bool vkShade::ConfigStore::save(std::filesystem::path filePath)
{
    std::ofstream file(filePath);
    if (!file.is_open())
    {
        Logger::error("Failed to open {} file for writing: {}", m_typeString, filePath.string());
        return false;
    }

    // Group by section
    std::map<std::string, std::vector<std::pair<std::string, std::string>>> sections;

    for (const auto& [key, value] : m_config)
    {
        // Split "Section::Key" back into parts
        size_t pos = key.find("::");
        if (pos == std::string::npos)
            continue;

        std::string section = key.substr(0, pos);
        std::string name = key.substr(pos + 2);

        sections[section].emplace_back(name, value);
    }

    // Write in specific order: unnamed section, vkShade, then alphabetical
    auto write_section = [&](const std::string& section_name)
    {
        auto it = sections.find(section_name);
        if (it != sections.end())
        {
            if (!section_name.empty())
                file << "[" << section_name << "]\n";

            for (const auto& [name, value] : it->second)
                file << name << " = " << value << "\n";

            file << "\n";
            sections.erase(it);
        }
    };

    write_section("");        // Unnamed/global section first
    write_section("vkShade"); // App settings second

    // Write remaining sections alphabetically
    for (auto it = sections.begin(); it != sections.end(); it++)
    {
        file << "[" << it->first << "]\n";
        for (const auto& [name, value] : it->second)
            file << name << " = " << value << "\n";

        if (std::next(it) != sections.end())
            file << "\n";
    }

    m_currentFile = filePath;
    Logger::info("Saved {} file: {}", m_typeString, filePath.string());
    return true;
}

bool vkShade::ConfigStore::save()
{
    if (!m_currentFile.empty())
        return this->save(m_currentFile);

    return false;
}
