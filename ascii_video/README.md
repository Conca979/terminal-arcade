# ASCII Video Player

High-performance 24-bit TrueColor ASCII video playback engine in the terminal.

## Overview

The ASCII Video Player transforms MP4 video files into real-time ANSI TrueColor ASCII art directly inside standard terminal emulators. It features dynamic resolution scaling that adapts immediately to terminal resizing or zoom (Ctrl +/-), aspect ratio compensation for monospace fonts, directional Sobel edge detection, and non-blocking interactive keyboard controls.

## Features

- Full 24-bit TrueColor ANSI escape sequence output with adjacent color run-length serialization.
- Line-clear escape sequence (`\x1b[K`) appended to each row to eliminate ghost artifacts upon terminal resizing.
- Dynamic terminal resolution scaling: automatically adapts on resize using `shutil.get_terminal_size()`.
- Geometric aspect ratio preservation: compensates for non-square monospace font cells (configurable ratio, default ~0.5).
- Perceptual luminance calculation based on ITU-R BT.601 standard: Y = 0.299R + 0.587G + 0.114B.
- Real-time contrast adjustment.
- Three glyph mapping modes:
  - Density ramp: maps luminance to characters (' .:-=+*#%@').
  - Directional Sobel edges: extracts gradients and discretizes orientations to '-', '|', '/', '\\'.
  - Hybrid mode: blends directional edges over smooth luminance shading.
- Single-call in-place screen updates using ANSI cursor home (`\x1b[H`) to eliminate flicker.
- Precision frame-pacing clock using `time.perf_counter()` to maintain accurate playback speed.
- Interactive keyboard controls on Windows and Unix platforms with immediate re-rendering during pause.

## Requirements

- Python 3.9+
- numpy >= 1.20.0
- opencv-python >= 4.5.0
- colorama >= 0.4.0
- pytest >= 7.0.0 (development)

Install dependencies:

```bash
pip install -r ascii_video/requirements.txt
```

## Usage

### Command Line Interface

Play an MP4 video file:

```bash
python -m ascii_video VID_20261022_230029_524.mp4
```

If no file argument is provided, the player automatically searches for `VID_20261022_230029_524.mp4` in the current directory and repository root:

```bash
python -m ascii_video
```

Generate and play a synthetic demonstration video:

```bash
python -m ascii_video --demo
```

### Options

- `video`: Path to the input MP4 file (positional argument).
- `--video PATH`: Explicit video file path.
- `--demo`: Generate and play a synthetic demonstration video.
- `-m, --mode {hybrid,density,edge}`: Glyph mapping mode (default: hybrid).
- `-w, --width WIDTH`: Override output width in characters (disables dynamic terminal width).
- `-H, --height HEIGHT`: Override output height in characters (disables dynamic terminal height).
- `-c, --contrast CONTRAST`: Contrast scaling multiplier (default: 1.0).
- `--edge-threshold THRESHOLD`: Sobel gradient magnitude threshold for edge detection (default: 40.0).
- `--char-aspect ASPECT`: Monospace character width-to-height ratio (default: 0.5).
- `--ramp RAMP`: Custom ASCII density ramp from dark to bright.
- `--invert`: Invert density ramp mapping.
- `--no-color, --mono`: Disable 24-bit TrueColor ANSI sequences (monochrome output).
- `-l, --loop`: Loop video playback continuously.
- `--fps FPS`: Override playback framerate.

### Interactive Controls

While video is playing in the terminal:

- `Space`: Pause / Resume playback.
- `M`: Toggle glyph mapping mode (Density -> Edge -> Hybrid).
- `+` or `=`: Increase contrast (+0.1).
- `-` or `_`: Decrease contrast (-0.1).
- `[` or `Left Arrow`: Seek backward 5 seconds.
- `]` or `Right Arrow`: Seek forward 5 seconds.
- `R`: Restart playback from beginning (frame 0).
- `Q` or `Esc`: Quit playback and restore terminal.

## Architecture and Modules

- `ascii_video/converter.py`: Frame resizing, perceptual luminance computation, Sobel edge filtering, glyph lookup, and ANSI TrueColor serialization.
- `ascii_video/player.py`: OpenCV video capture, non-blocking input handling, precision pacing, dynamic terminal size detection, and screen buffer rendering.
- `ascii_video/cli.py`: Command-line interface and argument parsing.
- `ascii_video/sample_generator.py`: Synthetic test video generator with moving geometric patterns and color waves.
- `ascii_video/tests/`: Automated unit and integration test suite.

## Running Tests

Execute unit and integration tests using pytest:

```bash
pytest ascii_video/tests
```
