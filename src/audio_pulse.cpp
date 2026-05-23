#include "audio_pulse.h"

#include <cstdlib>
#include <iostream>

namespace whisper_dictate {

void list_sources() {
    std::cout << "listing PulseAudio/PipeWire sources via pactl...\n";
    const int rc = std::system("pactl list short sources");
    if (rc != 0) {
        std::cerr << "warning: failed to run 'pactl list short sources'\n";
    }
}

}
