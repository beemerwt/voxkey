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
#include "transcriber_whisper.h"

namespace {

void print_version() {
    std::cout << "voxkey version " << VOXKEY_VERSION << "\n"
              << "build_type=" << VOXKEY_BUILD_TYPE << "\n"
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
    std::cout << "GPU enabled in config: " << (cfg.use_gpu ? "yes" : "no") << "\n";

    if (test_mic_seconds > 0) {
        try {
            auto samples = voxkey::record_seconds(cfg.input_source, test_mic_seconds);
            auto stats = voxkey::compute_audio_stats(samples);
            std::cout << "sample_count=" << stats.sample_count << " peak=" << stats.peak << " rms=" << stats.rms << "\n";
        } catch (const std::exception& e) {
            std::cerr << "test mic error: " << e.what() << "\n";
            return 1;
        }
        return 0;
    }
    if (!test_output.empty()) {
        try {
            voxkey::emit_output_text(test_output, cfg.output_mode);
        } catch (const std::exception& e) {
            std::cerr << "output error: " << e.what() << "\n";
            return 1;
        }
        return 0;
    }

    voxkey::Transcriber transcriber;
    if (!transcriber.load(cfg, std::cout)) {
        return 1;
    }

    std::mutex transcribe_mutex;
    voxkey::AudioRecorder recorder;
    std::atomic<bool> recording{false};
    std::atomic<bool> partial_running{false};
    std::thread partial_thread;

    auto stop_partial_thread = [&]() {
        partial_running = false;
        if (partial_thread.joinable()) {
            partial_thread.join();
        }
    };

    auto on_press = [&]() {
        if (recording.exchange(true)) return;
        std::cout << "recording start\n";
        recorder.start(cfg.input_source);
        partial_running = true;
        partial_thread = std::thread([&]() {
            const std::size_t window_samples = static_cast<std::size_t>(cfg.window_ms) * 16000 / 1000;
            std::atomic<bool> decode_done{true};
            std::thread decode_thread;
            while (partial_running.load()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(cfg.step_ms));
                if (!partial_running.load()) {
                    break;
                }
                if (decode_thread.joinable()) {
                    if (decode_done.load()) {
                        decode_thread.join();
                    } else {
                        std::cout << "partial transcript: skipped, previous decode still running\n";
                        continue;
                    }
                }
                auto window = recorder.last_samples_snapshot(window_samples);
                if (window.empty()) {
                    continue;
                }
                decode_done = false;
                decode_thread = std::thread([&, window = std::move(window)]() {
                    try {
                        std::lock_guard<std::mutex> lock(transcribe_mutex);
                        const std::string partial = transcriber.transcribe(window, cfg, true);
                        if (!partial.empty()) {
                            std::cout << "partial transcript: " << partial << "\n";
                        }
                    } catch (const std::exception& e) {
                        std::cerr << "partial transcript error: " << e.what() << "\n";
                    }
                    decode_done = true;
                });
            }
            if (decode_thread.joinable()) {
                decode_thread.join();
            }
        });
    };

    auto on_release = [&]() {
        if (!recording.exchange(false)) return;
        std::cout << "release detected\n";
        partial_running = false;
        recorder.stop_after_ms(cfg.post_release_ms);
        if (partial_thread.joinable()) {
            partial_thread.join();
        }
        std::cout << "post-release buffer complete\n";
        auto utterance = recorder.samples_snapshot();
        auto stats = voxkey::compute_audio_stats(utterance);
        std::cout << "utterance complete: samples=" << stats.sample_count << " peak=" << stats.peak << " rms=" << stats.rms << "\n";
        try {
            std::lock_guard<std::mutex> lock(transcribe_mutex);
            const std::string final_text = transcriber.transcribe(utterance, cfg, false);
            std::cout << "final transcript: " << final_text << "\n";
            if (!final_text.empty()) {
                voxkey::emit_output_text(final_text, cfg.output_mode);
            }
        } catch (const std::exception& e) {
            std::cerr << "final transcript error: " << e.what() << "\n";
        }
    };

    const int rc = voxkey::run_hotkey_loop(cfg.hotkey, on_press, on_release);
    stop_partial_thread();
    recorder.stop_now();
    return rc;
}
