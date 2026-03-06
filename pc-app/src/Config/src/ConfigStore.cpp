#include "Config/ConfigStore.h"

#include <fstream>

bool ConfigStore::loadConfig(AppConfig &config) {
    try {
        toml::table configTable = toml::parse_file(m_configPath.string());
        parseTomlTable(configTable, config);
        return true;
    } catch (const toml::parse_error &e) {
        return false;
    }
}

void ConfigStore::saveDefaultConfig() {
    toml::table configTable{
        {"system",
         toml::table{
             {"auto_start", false},
         }},
        {"serial", toml::table{{"com_port", "COM3"},
                               {"baud_rate", 115200},
                               {"invert_sliders", false}}},

        {"audio",
         toml::table{
             {"mute_buttons", false},
             {"num_channels", 2},
             {"channels",
              toml::array{
                  toml::table{{"channel", 0}, {"processes", "master"}},
                  toml::table{{"channel", 1},
                              {"processes",
                               toml::array{"chrome.exe", "spotify.exe"}}}}}}}};

    std::ofstream configFile(m_configPath);
    if (configFile.is_open()) {
        configFile << toml::toml_formatter(configTable);
        configFile.close();
    }
}

void ConfigStore::parseTomlTable(const toml::table &table, AppConfig &config) {
    config.auto_start = table["system"]["auto_start"].value_or(config.auto_start);
    config.serialConfig.com_port =
        table["serial"]["com_port"].value_or(config.serialConfig.com_port);
    config.serialConfig.baud_rate =
        table["serial"]["baud_rate"].value_or(config.serialConfig.baud_rate);
    config.serialConfig.invert_sliders =
        table["serial"]["invert_sliders"].value_or(
            config.serialConfig.invert_sliders);

    config.audioConfig.mute_buttons = table["audio"]["mute_buttons"].value_or(
        config.audioConfig.mute_buttons);
    config.audioConfig.num_channels = table["audio"]["num_channels"].value_or(
        config.audioConfig.num_channels);

    if (const auto *channels = table["audio"]["channels"].as_array()) {
        for (const auto &channelEntry : *channels) {
            if (const auto *channelTable = channelEntry.as_table()) {
                int channelIndex = (*channelTable)["channel"].value_or(-1);
                if (channelIndex >= 0) {
                    auto processesNode = (*channelTable)["processes"];

                    if (processesNode.is_string()) {
                        config.audioConfig
                            .processChannelMapping[processesNode.value_or(
                                L"")] = channelIndex;
                    } else if (const auto *processArray =
                                   processesNode.as_array()) {
                        for (const auto &proc : *processArray) {
                            if (proc.is_string()) {
                                config.audioConfig
                                    .processChannelMapping[proc.value_or(L"")] =
                                    channelIndex;
                            }
                        }
                    }
                }
            }
        }
    }
}