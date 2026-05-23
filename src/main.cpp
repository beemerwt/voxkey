#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "audio_pulse.h"
#include "config.h"
#include "config_build.h"
#include "hotkey_x11.h"
#include "output_x11.h"

void voxkey_transcriber_stub_message();

namespace {

void print_version() {
    std::cout << "voxkey version " << VOXKEY_VERSION << "\n"
              << "with_whisper=" << (VOXKEY_WITH_WHISPER ? "true" : "false") << "\n"
              << "cuda=" << (VOXKEY_CUDA ? "true" : "false") << "\n";
}

}

int main(int argc, char** argv) {
    std::string config_path;
    std::string test_output;
    int test_mic_seconds = 0;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--version") { print_version(); return 0; }
        if (arg == "--list-sources") { voxkey::list_sources(); return 0; }
        if (arg == "--self-test-config") { return voxkey::self_test_config(std::cout) ? 0 : 1; }
        if (arg == "--config" && i + 1 < argc) { config_path = argv[++i]; continue; }
        if (arg == "--test-mic" && i + 1 < argc) { test_mic_seconds = std::stoi(argv[++i]); continue; }
        if (arg == "--test-output" && i + 1 < argc) { test_output = argv[++i]; continue; }
        std::cerr << "error: unknown argument: " << arg << "\n";
        return 1;
    }

    print_version();
    std::cout << "CUDA build requested: " << (VOXKEY_CUDA ? "yes" : "no") << "\n";
    auto cfg = voxkey::load_config(config_path, std::cerr);
    voxkey::print_config(cfg, std::cout);

    if (test_mic_seconds > 0) {
        auto samples = voxkey::record_seconds(cfg.input_source, test_mic_seconds);
        auto stats = voxkey::compute_audio_stats(samples);
        std::cout << "sample_count=" << stats.sample_count << " peak=" << stats.peak << " rms=" << stats.rms << "\n";
        return 0;
    }
    if (!test_output.empty()) {
        voxkey::emit_output_text(test_output, cfg.output_mode);
        return 0;
    }

    voxkey_transcriber_stub_message();

    std::mutex m;
    std::vector<float> utterance;
    std::atomic<bool> recording{false};
    std::thread rec_thread;

    auto on_press = [&]() {
        if (recording.exchange(true)) return;
        utterance.clear();
        rec_thread = std::thread([&]() {
            while (recording.load()) {
                try {
                    auto chunk = voxkey::record_seconds(cfg.input_source, 1);
                    std::lock_guard<std::mutex> lock(m);
                    utterance.insert(utterance.end(), chunk.begin(), chunk.end());
                } catch (const std::exception& e) {
                    std::cerr << "recording error: " << e.what() << "\n";
                    recording = false;
                }
            }
        });
    };

    auto on_release = [&]() {
        if (!recording.exchange(false)) return;
        std::this_thread::sleep_for(std::chrono::milliseconds(cfg.post_release_ms));
        if (rec_thread.joinable()) rec_thread.join();
        std::lock_guard<std::mutex> lock(m);
        auto stats = voxkey::compute_audio_stats(utterance);
        std::cout << "utterance complete: samples=" << stats.sample_count << " peak=" << stats.peak << " rms=" << stats.rms << "\n";
    };

    return voxkey::run_hotkey_loop(cfg.hotkey, on_press, on_release);
}
