#pragma once

#include "Config/AppConfig.h"

#include <filesystem>
#include <toml++/toml.hpp>

class ConfigStore {
public:
    ConfigStore();
    ~ConfigStore() = default;

    ConfigStore(const ConfigStore &) = delete;
    ConfigStore &operator=(const ConfigStore &) = delete;
    ConfigStore(ConfigStore &&) = delete;
    ConfigStore &operator=(ConfigStore &&) = delete;

    bool loadConfig(AppConfig &config);

private:
    std::filesystem::path m_configPath;
    void saveDefaultConfig();
    void parseTomlTable(const toml::table &table, AppConfig &config);
};