#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace voxkey {

struct AudioStats {
    std::size_t sample_count = 0;
    float peak = 0.0f;
    float rms = 0.0f;
};

void list_sources();
std::vector<float> record_seconds(const std::string& source_name, int seconds);
AudioStats compute_audio_stats(const std::vector<float>& samples);

}  // namespace voxkey
