#pragma once

#include <ostream>
#include <string>

namespace voxkey {

struct Config {
    std::string model_path;
    std::string hotkey = "Pause";
    std::string input_source = "stable_input";
    std::string output_mode = "clipboard_paste";
    int post_release_ms = 350;
    int step_ms = 500;
    int window_ms = 5000;
    int threads = 8;
    std::string language = "en";
    bool use_gpu = true;
};

Config default_config();
Config load_config(const std::string& explicit_path, std::ostream& log);
std::string default_config_path();
void print_config(const Config& config, std::ostream& out);
bool self_test_config(std::ostream& out);

}  // namespace voxkey
