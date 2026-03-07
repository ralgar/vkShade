#pragma once

#include "config_store.hpp"

namespace vkShade
{
    class ConfigManager
    {
    public:
        ConfigManager()
        {
            // FIXME: Load from standard paths instead of hardcoded test path
            m_config = ConfigStore({"./config/vkshade.ini"});
            m_preset = ConfigStore({"./config/preset.ini"});
        }

        ConfigStore& app() { return m_config; }
        ConfigStore& preset() { return m_preset; }
    private:
        ConfigStore m_config;
        ConfigStore m_preset;
    };
} // namespace vkShade
