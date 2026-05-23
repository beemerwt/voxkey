# whisper-dictate

Hold-to-talk X11 dictation utility (Milestones 0-3 implemented in this PR).

## Dependencies (Debian/Ubuntu)

```bash
sudo apt install -y build-essential meson ninja-build pkg-config git libpulse-dev libx11-dev libxtst-dev
```

## Build

```bash
meson setup build
meson compile -C build
./build/whisper-dictate --version
```

## Config

Default config path:

`~/.config/whisper-dictate/config.conf`

Example:

```ini
model_path=$HOME/apps/whisper.cpp/models/ggml-base.en.bin
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
- `~/` is expanded in paths.
- Unknown keys and invalid values print clear warnings.

If using Pause through keyd, disable X autorepeat:

```bash
xset -r 127
```

## Current CLI (M0-M3)

- `whisper-dictate` → prints resolved config and starts X11 hotkey press/release loop.
- `whisper-dictate --config /path/to/config.conf` → use explicit config.
- `whisper-dictate --version` → print build info.
- `whisper-dictate --list-sources` → list sources via `pactl` (temporary for milestone 2).
- `whisper-dictate --self-test-config` → simple parser sanity check.

## Whisper backend status

With default `-Dwith_whisper=false`, app builds with a stub backend and prints:

`whisper backend not enabled`
