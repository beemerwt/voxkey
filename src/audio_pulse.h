#pragma once

#include <cstdint>
#include <atomic>
#include <mutex>
#include <string>
#include <thread>
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

class AudioRecorder {
public:
    AudioRecorder();
    ~AudioRecorder();

    AudioRecorder(const AudioRecorder&) = delete;
    AudioRecorder& operator=(const AudioRecorder&) = delete;

    void start(const std::string& source_name);
    void stop_after_ms(int milliseconds);
    void stop_now();
    bool is_running() const;
    std::vector<float> samples_snapshot() const;
    std::vector<float> last_samples_snapshot(std::size_t max_samples) const;

private:
    void worker(const std::string& source_name);

    mutable std::mutex mutex_;
    std::vector<float> samples_;
    std::thread thread_;
    std::atomic<bool> stop_requested_;
    std::atomic<bool> running_;
};

}  // namespace voxkey
