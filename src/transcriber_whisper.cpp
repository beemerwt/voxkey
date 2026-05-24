#include "transcriber_whisper.h"

#include <stdexcept>

#include <whisper.h>

namespace voxkey {

struct Transcriber::Impl {
    whisper_context* ctx = nullptr;
};

Transcriber::Transcriber() : impl_(new Impl()) {}

Transcriber::~Transcriber() {
    if (impl_->ctx) {
        whisper_free(impl_->ctx);
    }
    delete impl_;
}

bool Transcriber::load(const Config& config, std::ostream& log) {
    whisper_context_params cparams = whisper_context_default_params();
    cparams.use_gpu = config.use_gpu;

    impl_->ctx = whisper_init_from_file_with_params(config.model_path.c_str(), cparams);
    if (!impl_->ctx) {
        log << "error: failed to load whisper model: " << config.model_path << "\n";
        return false;
    }

    log << "model loaded: " << config.model_path << "\n";
    log << "GPU enabled in config: " << (config.use_gpu ? "yes" : "no") << "\n";
    return true;
}

std::string Transcriber::transcribe(const std::vector<float>& samples, const Config& config, bool partial) {
    if (!impl_->ctx) {
        throw std::runtime_error("whisper model is not loaded");
    }
    if (samples.empty()) {
        return {};
    }

    whisper_full_params params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
    params.print_progress = false;
    params.print_special = false;
    params.print_realtime = false;
    params.print_timestamps = false;
    params.translate = false;
    params.language = config.language.empty() ? "en" : config.language.c_str();
    params.n_threads = config.threads;
    params.no_context = partial;
    params.single_segment = partial;

    const int rc = whisper_full(impl_->ctx, params, samples.data(), static_cast<int>(samples.size()));
    if (rc != 0) {
        throw std::runtime_error("whisper_full failed");
    }

    std::string text;
    const int n_segments = whisper_full_n_segments(impl_->ctx);
    for (int i = 0; i < n_segments; ++i) {
        const char* segment = whisper_full_get_segment_text(impl_->ctx, i);
        if (segment) {
            text += segment;
        }
    }
    return text;
}

}  // namespace voxkey
