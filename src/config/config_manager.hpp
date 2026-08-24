#pragma once

#include "config_store.hpp"
#include "platform/file_watcher.hpp"

namespace vkShade
{
    class ConfigManager
    {
    public:
        ConfigManager();

        ConfigStore& app() { return m_config; }
        ConfigStore& internal() { return m_internal; }
        ConfigStore& preset() { return m_preset; }

        void update();

    private:
        ConfigStore m_config;
        ConfigStore m_internal;
        ConfigStore m_preset;

        std::unique_ptr<Platform::FileWatcher> m_configWatcher;
        std::unique_ptr<Platform::FileWatcher> m_presetWatcher;

        void load_config_file();
        void load_preset_file();
    };
} // namespace vkShade
