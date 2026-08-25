#include "config_manager.hpp"

#include "core/logger.hpp"
#include "platform/file_watcher.hpp"

#include "config_globals.hpp"

vkShade::ConfigManager::ConfigManager()
    : m_config(ConfigStore(ConfigStore::Type::Config)),
      m_internal(ConfigStore(ConfigStore::Type::Internal)),
      m_preset(ConfigStore(ConfigStore::Type::Preset))
{
    load_config_file();
    load_preset_file();
}

void vkShade::ConfigManager::load_config_file()
{
    // Custom config file path
    const char* customConfigEnv  = std::getenv("VKSHADE_CONFIG_FILE");
    std::string customConfigFile = customConfigEnv ? std::string(customConfigEnv) : "";

    // User config file path
    const char* xdgDataEnv            = std::getenv("XDG_DATA_HOME");
    std::string userDefaultConfigFile = xdgDataEnv ? std::string(xdgDataEnv) + "/vkShade/vkShade.ini"
                                                   : std::string(std::getenv("HOME")) + "/.local/share/vkShade/vkShade.ini";

    const char* xdgConfigEnv         = std::getenv("XDG_CONFIG_HOME");
    std::string userCustomConfigFile = xdgConfigEnv ? std::string(xdgConfigEnv) + "/vkShade/vkShade.ini"
                                                    : std::string(std::getenv("HOME")) + "/.config/vkShade/vkShade.ini";

    // Allowed config paths
    const std::array<std::string, 7> configCandidates = {
        customConfigFile,                                   // Custom config (VKSHADE_CONFIG_FILE=/path/to/vkShade.ini)
        "vkShade.ini",                                      // Per-game config
        userCustomConfigFile,                               // User custom config
        userDefaultConfigFile,                              // User default config
        std::string(SYSCONFDIR) + "/vkShade.ini",           // System-wide custom config
        std::string(SYSCONFDIR) + "/vkShade/vkShade.ini",   // System-wide custom config (alternative)
        std::string(DATADIR) + "/vkShade/vkShade.ini",      // System-wide default config
    };

    for (const auto& filePath : configCandidates)
    {
        if (m_config.load(filePath))
        {
            // Successfully loaded candidate
            return;
        }
    }

    Logger::warn("No config file found");
}

void vkShade::ConfigManager::load_preset_file()
{
    // Custom config file path
    const char* customPresetEnv  = std::getenv("VKSHADE_PRESET_FILE");
    std::string customPresetFile = customPresetEnv ? std::string(customPresetEnv) : "";

    // User config file path
    const char* xdgDataEnv            = std::getenv("XDG_DATA_HOME");
    std::string userDefaultPresetFile = xdgDataEnv ? std::string(xdgDataEnv) + "/vkShade/ReShade.ini"
                                                   : std::string(std::getenv("HOME")) + "/.local/share/vkShade/ReShade.ini";

    const char* xdgPresetEnv         = std::getenv("XDG_CONFIG_HOME");
    std::string userCustomPresetFile = xdgPresetEnv ? std::string(xdgPresetEnv) + "/vkShade/ReShade.ini"
                                                    : std::string(std::getenv("HOME")) + "/.config/vkShade/ReShade.ini";

    // Allowed config paths
    const std::array<std::string, 7> presetCandidates = {
        customPresetFile,                                   // Custom preset (VKSHADE_PRESET_FILE=/path/to/ReShade.ini)
        "ReShade.ini",                                      // Per-game preset
        userCustomPresetFile,                               // User custom preset
        userDefaultPresetFile,                              // User default preset
        std::string(SYSCONFDIR) + "/ReShade.ini",           // System-wide custom preset
        std::string(SYSCONFDIR) + "/vkShade/ReShade.ini",   // System-wide custom preset (alternative)
        std::string(DATADIR) + "/vkShade/ReShade.ini",      // System-wide default preset
    };

    for (const auto& filePath : presetCandidates)
    {
        if (m_preset.load(filePath))
        {
            // Successfully loaded candidate
            return;
        }
    }

    Logger::warn("No preset file found");
}

void vkShade::ConfigManager::update()
{
    m_config.update();
    m_preset.update();
}
