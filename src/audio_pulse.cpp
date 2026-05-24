#include "audio_pulse.h"

#include <pulse/simple.h>
#include <pulse/error.h>

#include <algorithm>
#include <chrono>
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

AudioRecorder::AudioRecorder() : stop_requested_(false), running_(false) {}

AudioRecorder::~AudioRecorder() {
    stop_now();
}

void AudioRecorder::start(const std::string& source_name) {
    stop_now();
    {
        std::lock_guard<std::mutex> lock(mutex_);
        samples_.clear();
    }
    stop_requested_ = false;
    running_ = true;
    thread_ = std::thread(&AudioRecorder::worker, this, source_name);
}

void AudioRecorder::stop_after_ms(int milliseconds) {
    if (milliseconds > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
    }
    stop_now();
}

void AudioRecorder::stop_now() {
    stop_requested_ = true;
    if (thread_.joinable()) {
        thread_.join();
    }
    running_ = false;
}

bool AudioRecorder::is_running() const {
    return running_.load();
}

std::vector<float> AudioRecorder::samples_snapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return samples_;
}

std::vector<float> AudioRecorder::last_samples_snapshot(std::size_t max_samples) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (samples_.size() <= max_samples) {
        return samples_;
    }
    return std::vector<float>(samples_.end() - static_cast<std::ptrdiff_t>(max_samples), samples_.end());
}

void AudioRecorder::worker(const std::string& source_name) {
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
        running_ = false;
        std::cerr << "recording error: failed to open pulse source: " << pa_strerror(error) << "\n";
        return;
    }

    std::cout << "audio source opened: " << (source_name.empty() ? "(default)" : source_name) << "\n";

    const std::size_t chunk_samples = ss.rate / 10;
    std::vector<int16_t> pcm(chunk_samples);
    while (!stop_requested_.load()) {
        if (pa_simple_read(pa, pcm.data(), pcm.size() * sizeof(int16_t), &error) < 0) {
            std::cerr << "recording error: failed to read pulse samples: " << pa_strerror(error) << "\n";
            break;
        }

        std::lock_guard<std::mutex> lock(mutex_);
        samples_.reserve(samples_.size() + pcm.size());
        for (auto s : pcm) {
            samples_.push_back(static_cast<float>(s) / 32768.0f);
        }
    }

    pa_simple_free(pa);
    running_ = false;
}

}  // namespace voxkey
