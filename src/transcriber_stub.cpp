#include "transcriber_whisper.h"

#include <iostream>

namespace voxkey {

struct Transcriber::Impl {};

Transcriber::Transcriber() : impl_(new Impl()) {}

Transcriber::~Transcriber() {
    delete impl_;
}

bool Transcriber::load(const Config&, std::ostream& log) {
    log << "whisper backend not enabled\n";
    return true;
}

std::string Transcriber::transcribe(const std::vector<float>&, const Config&, bool partial) {
    if (!partial) {
        std::cout << "whisper backend not enabled\n";
    }
    return {};
}

}  // namespace voxkey
