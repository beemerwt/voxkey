#pragma once

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <string>

namespace whisper_dictate {

inline std::string trim(std::string value) {
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));

    value.erase(std::find_if(value.rbegin(), value.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), value.end());

    return value;
}

inline bool parse_bool(const std::string& value, bool& out) {
    if (value == "true" || value == "1" || value == "yes" || value == "on") {
        out = true;
        return true;
    }
    if (value == "false" || value == "0" || value == "no" || value == "off") {
        out = false;
        return true;
    }
    return false;
}

inline std::string expand_tilde(const std::string& path) {
    if (path.rfind("~/", 0) != 0) {
        return path;
    }
    const char* home = std::getenv("HOME");
    if (!home) {
        return path;
    }
    return std::string(home) + path.substr(1);
}

}  // namespace whisper_dictate
