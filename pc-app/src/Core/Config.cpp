#include "Config.h"
#include <filesystem>

Config::Config() : m_configFilePath("config.yaml") { loadConfig(); }

Config::Config(const std::string &configFilePath)
    : m_configFilePath(configFilePath) {
    loadConfig();
}

void Config::loadConfig() {
    try {
        // Find config file in current working directory
        auto yamlPath = std::filesystem::current_path() / m_configFilePath;
        YAML::Node config = YAML::LoadFile(yamlPath.string());

        // Parse required configuration fields
        if (config["com_port"]) {
            m_comPort = config["com_port"].as<std::string>();
        } else {
            throw std::runtime_error("Missing 'com_port' in config file.");
        }

        if (config["baud_rate"]) {
            m_baudRate = config["baud_rate"].as<int>();
        } else {
            throw std::runtime_error("Missing 'baud_rate' in config file.");
        }

        if (config["invert_slider"]) {
            m_invertSlider = config["invert_slider"].as<bool>();
        } else {
            throw std::runtime_error("Missing 'invert_slider' in config file.");
        }

        if (config["auto_start"]) {
            m_autoStart = config["auto_start"].as<bool>();
        } else {
            throw std::runtime_error("Missing 'auto_start' in config file.");
        }

        if (config["mute_buttons"]) {
            m_muteButtons = config["mute_buttons"].as<bool>();
        } else {
            throw std::runtime_error("Missing 'mute_buttons' in config file.");
        }

        // Parse channel to applications mapping
        if (config["channel_apps"]) {
            for (const auto &channel : config["channel_apps"]) {
                int channelNumber = channel.first.as<int>();
                std::vector<std::string> apps =
                    channel.second.as<std::vector<std::string>>();
                m_channelApps[channelNumber] = apps;
            }
        } else {
            throw std::runtime_error("Missing 'channel_apps' in config file.");
        }
    } catch (const YAML::Exception &e) {
        throw std::runtime_error("YAML parsing error: " +
                                 std::string(e.what()));
    } catch (const std::exception &e) {
        throw std::runtime_error("Error loading config: " +
                                 std::string(e.what()));
    }
}
