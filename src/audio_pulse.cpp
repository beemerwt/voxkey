#include "audio_pulse.h"

#include <pulse/simple.h>
#include <pulse/error.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

namespace voxkey {

void list_sources() {
    std::cout << "listing PulseAudio/PipeWire sources via pactl...\n";
    const int rc = std::system("pactl list short sources");
    if (rc != 0) {
        std::cerr << "warning: failed to run 'pactl list short sources'\n";
    }
}

std::vector<float> record_seconds(const std::string& source_name, int seconds) {
    if (seconds <= 0) {
        throw std::runtime_error("record duration must be > 0 seconds");
    }

    pa_sample_spec ss{};
    ss.format = PA_SAMPLE_S16LE;
    ss.rate = 16000;
    ss.channels = 1;

    int error = 0;
    pa_simple* pa = pa_simple_new(
        nullptr,
        "voxkey",
        PA_STREAM_RECORD,
        source_name.empty() ? nullptr : source_name.c_str(),
        "record",
        &ss,
        nullptr,
        nullptr,
        &error);

    if (!pa) {
        throw std::runtime_error(std::string("failed to open pulse source: ") + pa_strerror(error));
    }

    const std::size_t total_samples = static_cast<std::size_t>(ss.rate) * static_cast<std::size_t>(seconds);
    std::vector<int16_t> pcm(total_samples);
    if (pa_simple_read(pa, pcm.data(), pcm.size() * sizeof(int16_t), &error) < 0) {
        pa_simple_free(pa);
        throw std::runtime_error(std::string("failed to read pulse samples: ") + pa_strerror(error));
    }
    pa_simple_free(pa);

    std::vector<float> out;
    out.reserve(pcm.size());
    for (auto s : pcm) {
        out.push_back(static_cast<float>(s) / 32768.0f);
    }
    return out;
}

AudioStats compute_audio_stats(const std::vector<float>& samples) {
    AudioStats stats;
    stats.sample_count = samples.size();
    if (samples.empty()) {
        return stats;
    }

    double sum_sq = 0.0;
    float peak = 0.0f;
    for (float s : samples) {
        peak = std::max(peak, std::abs(s));
        sum_sq += static_cast<double>(s) * static_cast<double>(s);
    }

    stats.peak = peak;
    stats.rms = static_cast<float>(std::sqrt(sum_sq / static_cast<double>(samples.size())));
    return stats;
}

}  // namespace voxkey
