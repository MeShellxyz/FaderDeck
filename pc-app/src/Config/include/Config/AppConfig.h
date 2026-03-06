#pragma once

#include <string>
#include <unordered_map>

struct SerialConfig {
    std::string com_port{"COM3"};
    int baud_rate{115200};
    bool invert_sliders{false};
};
struct AudioConfig {
    bool mute_buttons{false};
    int num_channels{0};
    std::unordered_map<std::wstring, int> processChannelMapping{0};
};

struct AppConfig {
    bool auto_start{false};
    SerialConfig serialConfig;
    AudioConfig audioConfig;
};