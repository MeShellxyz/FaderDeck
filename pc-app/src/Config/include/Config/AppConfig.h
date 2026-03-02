#pragma once

#include <string>
#include <unordered_map>

struct SerialConfig {
    std::string com_port;
    int baud_rate;
    bool invert_sliders;
};
struct AudioConfig {
    bool mute_buttons;
    int num_channels;
    std::unordered_map<std::wstring, int> processChannelMapping;
};

struct AppConfig {
    SerialConfig serialConfig;
    AudioConfig audioConfig;
};