#pragma once

#include <string>
#include <vector>

struct SerialConfig {
    std::string com_port;
    int baud_rate;
};
struct AudioConfig {
    bool invert_sliders;
    bool mute_buttons;
    int num_channels;
    std::vector<std::vector<std::string>> channel_apps;
};

struct AppConfig {
    SerialConfig serialConfig;
    AudioConfig audioConfig;
};