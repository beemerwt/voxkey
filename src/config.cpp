#include "config.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#include "config_build.h"
#include "util.h"

namespace whisper_dictate {
namespace {

bool parse_int(const std::string& key, const std::string& value, int& out, std::ostream& log) {
    try {
        size_t pos = 0;
        int parsed = std::stoi(value, &pos);
        if (pos != value.size()) {
            log << "warning: invalid integer for key '" << key << "': '" << value << "'\n";
            return false;
        }
        out = parsed;
        return true;
    } catch (...) {
        log << "warning: invalid integer for key '" << key << "': '" << value << "'\n";
        return false;
    }
}

}

Config default_config() {
    Config cfg;
    if (std::string(WHISPER_DICTATE_DEFAULT_MODEL).size() > 0) {
        cfg.model_path = WHISPER_DICTATE_DEFAULT_MODEL;
    } else {
        cfg.model_path = "~/models/ggml-base.en.bin";
    }
    return cfg;
}

std::string default_config_path() {
    return expand_tilde("~/.config/voxkey/config.conf");
}

Config load_config(const std::string& explicit_path, std::ostream& log) {
    Config cfg = default_config();
    const std::string path = explicit_path.empty() ? default_config_path() : explicit_path;

    std::ifstream in(path);
    if (!in.is_open()) {
        log << "info: config file not found at '" << path << "', using defaults\n";
        cfg.model_path = expand_tilde(cfg.model_path);
        return cfg;
    }

    std::string line;
    int line_no = 0;
    while (std::getline(in, line)) {
        ++line_no;
        std::string t = trim(line);
        if (t.empty() || t[0] == '#') {
            continue;
        }
        const auto eq = t.find('=');
        if (eq == std::string::npos) {
            log << "warning: invalid config line " << line_no << " (missing '='): " << t << "\n";
            continue;
        }

        const std::string key = trim(t.substr(0, eq));
        const std::string value = trim(t.substr(eq + 1));

        if (key == "model_path") cfg.model_path = expand_tilde(value);
        else if (key == "hotkey") cfg.hotkey = value;
        else if (key == "input_source") cfg.input_source = value;
        else if (key == "output_mode") cfg.output_mode = value;
        else if (key == "post_release_ms") parse_int(key, value, cfg.post_release_ms, log);
        else if (key == "step_ms") parse_int(key, value, cfg.step_ms, log);
        else if (key == "window_ms") parse_int(key, value, cfg.window_ms, log);
        else if (key == "threads") parse_int(key, value, cfg.threads, log);
        else if (key == "language") cfg.language = value;
        else if (key == "use_gpu") {
            bool parsed = false;
            if (!parse_bool(value, parsed)) {
                log << "warning: invalid bool for key 'use_gpu': '" << value << "'\n";
            } else {
                cfg.use_gpu = parsed;
            }
        } else {
            log << "warning: unknown config key '" << key << "'\n";
        }
    }

    cfg.model_path = expand_tilde(cfg.model_path);
    return cfg;
}

void print_config(const Config& c, std::ostream& out) {
    out << "resolved config:\n"
        << "  model_path=" << c.model_path << "\n"
        << "  hotkey=" << c.hotkey << "\n"
        << "  input_source=" << c.input_source << "\n"
        << "  output_mode=" << c.output_mode << "\n"
        << "  post_release_ms=" << c.post_release_ms << "\n"
        << "  step_ms=" << c.step_ms << "\n"
        << "  window_ms=" << c.window_ms << "\n"
        << "  threads=" << c.threads << "\n"
        << "  language=" << c.language << "\n"
        << "  use_gpu=" << (c.use_gpu ? "true" : "false") << "\n";
}

bool self_test_config(std::ostream& out) {
    const auto p = expand_tilde("~/tmp");
    if (p.find('/') == std::string::npos) {
        out << "self-test failed: tilde expansion\n";
        return false;
    }
    out << "self-test passed\n";
    return true;
}

}  // namespace whisper_dictate
