#include "config_store.hpp"

#include <fstream>
#include <map>

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

vkShade::ConfigStore::ConfigStore(std::vector<std::filesystem::path> filePaths)
    : m_parser(ConfigParser()),
      m_filePaths(filePaths)
{
    load();
}

void vkShade::ConfigStore::load()
{
    for (const auto& file : m_filePaths)
    {
        if (!std::filesystem::exists(file))
            return; // Silently skip non-existent files

        // Parse the INI file
        int result = ini_parse(file.string().c_str(), config_ini_handler, &m_config);
        if (result != 0)
            spdlog::error("Failed to load or parse config file: {}", file.c_str());
    }
}

void vkShade::ConfigStore::save() const
{
    std::ofstream file(m_filePaths.back());
    if (!file.is_open())
    {
        spdlog::error("Failed to open config file for writing: {}", m_filePaths.back().string());
        return;
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

    spdlog::trace("Saved configuration to: {}", m_filePaths.back().string());
}
