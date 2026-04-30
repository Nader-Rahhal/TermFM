# TermFM
A terminal radio player with a TUI built using [FTXUI](https://github.com/ArthurSonzogni/FTXUI). Streams MP3 radio stations sourced from [radio-browser.info](https://www.radio-browser.info).

## Dependencies
- **CMake** 3.14+
- **ffplay** (part of ffmpeg) — used for audio playback

```bash
# macOS
brew install ffmpeg
# Ubuntu/Debian
sudo apt install ffmpeg
```

## Quick installation

### macOS
```bash
curl -L https://github.com/Nader-Rahhal/TermFM/releases/latest/download/termfm-macos -o termfm && chmod +x termfm && sudo mv termfm /usr/local/bin/
```

### Linux
```bash
curl -L https://github.com/Nader-Rahhal/TermFM/releases/latest/download/termfm-linux -o termfm && chmod +x termfm && sudo mv termfm /usr/local/bin/
```

Then run with `termfm`.

## Build from source
```bash
cmake -B build
cmake --build build
./build/termfm
```
FTXUI and nlohmann/json are fetched automatically by CMake.

## Usage
```bash
termfm
```

| Key | Action |
|-----|--------|
| `Tab` | Switch search mode (By Name / By Country / By Language) |
| `Enter` | Search (in search bar) or play (in station list) |
| `↑ ↓` | Navigate station list |
| `S` | Stop playback |
| `Q` | Quit |
