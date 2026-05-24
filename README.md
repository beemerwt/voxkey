# voxkey

Hold-to-talk X11 dictation utility for Debian-compatible desktops.

This app uses Meson + Ninja. It can build without whisper.cpp for hotkey, audio, and output development; when `-Dwith_whisper=true` is used, it links directly against an externally built whisper.cpp tree.

## Dependencies

```bash
sudo apt install -y \
  build-essential \
  meson \
  ninja-build \
  pkg-config \
  git \
  libpulse-dev \
  libx11-dev \
  libxtst-dev
```

CUDA builds also require a compatible NVIDIA driver and CUDA toolkit. CUDA support is provided by whisper.cpp/ggml, not by voxkey itself.

## Build Without Whisper

```bash
cd ~/projects/voxkey
meson setup build
meson compile -C build
./build/voxkey --version
```

With default `-Dwith_whisper=false`, voxkey builds a stub transcriber and prints `whisper backend not enabled`.

## Build With Whisper

Build whisper.cpp separately with its upstream build system:

```bash
git clone https://github.com/ggml-org/whisper.cpp ~/apps/whisper.cpp
cd ~/apps/whisper.cpp
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
```

Then build voxkey against the resulting libraries:

```bash
cd ~/projects/voxkey
meson setup build-whisper \
  -Dwith_whisper=true \
  -Dwhisper_include_dir=$HOME/apps/whisper.cpp/include \
  -Dwhisper_lib_dir=$HOME/apps/whisper.cpp/build/src

meson compile -C build-whisper
```

## CUDA Build

Build whisper.cpp with CUDA first:

```bash
cd ~/apps/whisper.cpp
cmake -S . -B build-cuda \
  -DCMAKE_BUILD_TYPE=Release \
  -DGGML_CUDA=ON

cmake --build build-cuda -j"$(nproc)"
```

Then configure voxkey with CUDA expected:

```bash
cd ~/projects/voxkey
meson setup build-cuda \
  -Dwith_whisper=true \
  -Dcuda=true \
  -Dwhisper_include_dir=$HOME/apps/whisper.cpp/include \
  -Dwhisper_lib_dir=$HOME/apps/whisper.cpp/build-cuda/src

meson compile -C build-cuda
```

If `-Dcuda=true` is set, Meson requires `-Dwith_whisper=true` and a discoverable `ggml-cuda` library.

## Model

Start with a small model for low latency, such as `ggml-base.en.bin` or `ggml-tiny.en.bin`, downloaded through whisper.cpp's model scripts.

## Config

Default config path:

`~/.config/voxkey/config.conf`

Example:

```ini
model_path=/home/beemer/apps/whisper.cpp/models/ggml-base.en.bin
hotkey=Pause
input_source=stable_input
output_mode=clipboard_paste
post_release_ms=350
step_ms=500
window_ms=5000
threads=8
language=en
use_gpu=true
```

Notes:

- Format is `KEY=VALUE`.
- Blank lines and lines starting with `#` are ignored.
- Leading and trailing whitespace around keys and values is trimmed.
- `~/` is expanded in paths.
- Unknown keys and invalid values print clear warnings.
- Valid output modes are `clipboard_only`, `clipboard_paste`, and `stdout`.

If using Pause through keyd, X autorepeat may need to be disabled for the Pause key:

```bash
xset -r 127
```

voxkey does not modify your keymap automatically.

## CLI

- `voxkey` runs the foreground X11 hotkey loop.
- `voxkey --config /path/to/config.conf` uses an explicit config file.
- `voxkey --list-sources` lists PulseAudio/PipeWire source names via `pactl`.
- `voxkey --test-mic 5` records five seconds and prints sample count, peak, and RMS.
- `voxkey --test-output "hello"` emits text according to `output_mode`.
- `voxkey --self-test-config` runs a small config parser sanity check.
- `voxkey --version` prints version, build type, whisper, and CUDA info.

## Current Milestones

Implemented:

- Meson project skeleton and config parser.
- PulseAudio/PipeWire source listing and test recording.
- X11 global hotkey press/release detection with autorepeat filtering.
- Continuous in-memory recording with a post-release tail.
- Direct whisper.cpp transcription path when enabled.
- Diagnostic partial decoding every `step_ms` over the last `window_ms`.
- X11 clipboard ownership plus Ctrl+V paste via XTest.
- CUDA build option that expects upstream CUDA-enabled whisper.cpp/ggml libraries.
