#pragma once

#include "config_store.hpp"

namespace vkShade
{
    class ConfigManager
    {
    public:
        ConfigManager();

        ConfigStore& app() { return m_config; }
        ConfigStore& internal() { return m_internal; }
        ConfigStore& preset() { return m_preset; }

    private:
        ConfigStore m_config;
        ConfigStore m_internal;
        ConfigStore m_preset;
    };
} // namespace vkShade
