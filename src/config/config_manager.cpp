#include "config_manager.hpp"

#include <spdlog/spdlog.h>

#include "config_globals.hpp"

vkShade::ConfigManager::ConfigManager()
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
        customConfigFile,                                   // Custom config (VKSHADE_CONFIG_FILE=/path/to/vkShade.conf)
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
            spdlog::info("Loaded config file: {}", filePath);
            return;
        }
    }

    spdlog::warn("No config file found");
}
