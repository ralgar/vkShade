#pragma once

#include <filesystem>

#include <spdlog/spdlog.h>

#include "config_store.hpp"

namespace vkShade
{
    class ConfigManager
    {
    public:
        ConfigManager();

        ConfigStore& app() { return m_config; }
        ConfigStore& preset() { return m_preset; }

    private:
        ConfigStore m_config;
        ConfigStore m_preset;
    };
} // namespace vkShade
