#pragma once

#include <ostream>
#include <string>
#include <vector>

#include "config.h"

namespace voxkey {

class Transcriber {
public:
    Transcriber();
    ~Transcriber();

    Transcriber(const Transcriber&) = delete;
    Transcriber& operator=(const Transcriber&) = delete;

    bool load(const Config& config, std::ostream& log);
    std::string transcribe(const std::vector<float>& samples, const Config& config, bool partial);

private:
    struct Impl;
    Impl* impl_;
};

}  // namespace voxkey
