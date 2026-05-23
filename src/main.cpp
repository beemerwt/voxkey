#include <iostream>
#include <string>

#include "audio_pulse.h"
#include "config.h"
#include "config_build.h"
#include "hotkey_x11.h"

void whisper_dictate_transcriber_stub_message();

namespace {

void print_version() {
    std::cout << "whisper-dictate version " << WHISPER_DICTATE_VERSION << "\n"
              << "with_whisper=" << (WHISPER_DICTATE_WITH_WHISPER ? "true" : "false") << "\n"
              << "cuda=" << (WHISPER_DICTATE_CUDA ? "true" : "false") << "\n";
}

}

int main(int argc, char** argv) {
    std::string config_path;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--version") {
            print_version();
            return 0;
        }
        if (arg == "--list-sources") {
            whisper_dictate::list_sources();
            return 0;
        }
        if (arg == "--self-test-config") {
            return whisper_dictate::self_test_config(std::cout) ? 0 : 1;
        }
        if (arg == "--config" && i + 1 < argc) {
            config_path = argv[++i];
            continue;
        }
        std::cerr << "error: unknown argument: " << arg << "\n";
        return 1;
    }

    print_version();
    std::cout << "CUDA build requested: " << (WHISPER_DICTATE_CUDA ? "yes" : "no") << "\n";

    auto cfg = whisper_dictate::load_config(config_path, std::cerr);
    whisper_dictate::print_config(cfg, std::cout);

    whisper_dictate_transcriber_stub_message();

    return whisper_dictate::run_hotkey_loop(cfg.hotkey);
}
