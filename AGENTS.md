# Terminal Studio in C — Developer & Agent Architecture Guide

A modular, high-performance collection of classic games, ASCII art, 3D terminal graphics, simulations, and visual demos written in C for Windows console environments with 60 FPS flicker-free ANSI color rendering.

---

## Project Vision & Roadmap

Terminal Studio is evolving into a unified terminal creative environment. The long-term scope encompasses:
- Interactive console games and puzzle systems
- Procedural and static ASCII / ANSI artwork visualizers
- 3D terminal graphics and software rendering pipelines
- Algorithmic simulations (cellular automata, particle fields, physics models)
- Visual demos and shader-style ANSI effects

---

## Standard 5-Module Game Architecture

Every game in `games/<game_name>/` follows a strict modular 5-component layout:

```
games/<game_name>/
├── include/
│   ├── config.h        # Constants, board sizes, enums, game states, action codes
│   ├── game.h          # Data structures, GameState model, pure logic prototypes
│   ├── input.h         # Input action mapping interface
│   ├── platform.h      # Console mode, QPC high-res timer, sleep interface
│   └── render.h        # ANSI TrueColor frame rendering interface
├── src/
│   ├── config.c        # (Optional) Static asset tables, dictionaries, word lists
│   ├── game.c          # Pure game logic, physics, board state, rules, score calculation
│   ├── input.c         # Non-blocking key decoding (_kbhit / _getch) -> GameAction
│   ├── platform.c      # Windows console VT100, UTF-8, QPC timing, cleanup hooks
│   ├── render.c        # Flicker-free TrueColor ANSI rendering (viewport budget <= 27 lines)
│   └── main.c          # 60 FPS game loop (Init -> Input -> Update -> Render -> Save)
├── test_<game_name>.c  # Headless automated unit tests (pure logic / no console GUI)
├── Makefile            # Standalone build (all, test, clean targets)
└── README.md           # User guide, gameplay rules, and player controls
```

### Module Responsibilities

1. **`config.h`**:
   - Defines all game constants (board dimensions, physics constants, presets, tick rates).
   - Declares state enums (`RoundState`, `PlayState`) and input semantic codes (`GameAction`).
2. **`game.h` / `game.c`**:
   - Encapsulates pure state (`GameState`) and deterministic simulation logic.
   - Independent of rendering and direct console I/O; 100% testable in headless suites.
   - Handles game reset, physics integration, collisions, rule evaluation, score calculation, and savegame serialization.
3. **`input.h` / `input.c`**:
   - Non-blocking keyboard decoding using `_kbhit()` and `_getch()`.
   - Mouse support (e.g. aiming/clicking in Breaker or Block) decoded via Win32 `ReadConsoleInputW` / `MOUSE_EVENT`.
   - Handles extended key codes (arrows: `0` or `224` prefix).
   - Normalizes raw events into semantic `GameAction` enums.
4. **`platform.h` / `platform.c`**:
   - Windows Console API terminal configuration and lifecycle management.
   - Enables virtual terminal sequences (`ENABLE_VIRTUAL_TERMINAL_PROCESSING`).
   - High-resolution timing via `QueryPerformanceCounter` (QPC).
   - Cleanup guarantees on process exit or signal interrupts.
5. **`render.h` / `render.c`**:
   - TrueColor ANSI rendering (`\033[38;2;R;G;Bm` fg, `\033[48;2;R;G;Bm` bg).
   - Viewport budgets matched to standard target terminal environment (Width: 120, Height: 30):
     - Height strictly `<= 27 lines` to prevent vertical scrolling/jitter on standard 30-line terminals (preserving comfort padding).
     - Compact mini-games (e.g. 2048, Snake, Wordle) remain `<= 21 lines`.
     - Taller multi-card / cascade games (e.g. Solitaire) use up to 27 lines (16 tableau rows).
     - Width up to 115 columns (e.g. Solitaire at 95 cols) fitting within 120 columns.
   - Dynamic visible length calculation (`visible_length()`) to ignore ANSI escapes and UTF-8 continuation bytes for precise box alignment.
   - Appends `\033[K` before newlines to clear trailing characters from prior frames.
6. **`main.c`**:
   - Coordinates game lifecycle: `platform_init()` -> `render_init()` -> `game_init()` -> 60 FPS Loop -> `game_save_scores()` -> `render_cleanup()` -> `platform_cleanup()`.
7. **`test_<game>.c`**:
   - Automated headless regression tests using standard `<assert.h>`.

---

## Technical Constraints & Implementation Guidelines

### 1. Language Standards & Formatting
- **Standard**: C99 / C11 pure standard library + Windows Console API.
- **Indentation**: Strict **2 spaces** indentation throughout the entire codebase (no tabs).
- **Naming Conventions**: `snake_case` for functions/variables, `UPPER_SNAKE_CASE` for constants/macros, `PascalCase` for typedef structs/enums.

### 2. Windows Console & VT100 ANSI Setup
All games must initialize and restore console state cleanly:
- **Output Mode**: Enable `ENABLE_VIRTUAL_TERMINAL_PROCESSING | ENABLE_PROCESSED_OUTPUT`.
- **Input Mode**: Disable `ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT` for immediate non-blocking raw keypresses (enable `ENABLE_MOUSE_INPUT | ENABLE_EXTENDED_FLAGS` when mouse support is required).
- **Code Page**: Set UTF-8 output code page: `SetConsoleOutputCP(CP_UTF8)`.
- **Alternate Buffer & Cursor**: Enter alternate buffer `\033[?1049h` and hide cursor `\033[?25l` on init; restore cursor `\033[?25h` and exit buffer `\033[?1049l` on cleanup.

### 3. Cleanup Guarantees
Console state MUST be restored on exit to prevent bricking the user's terminal:
- Register `atexit(platform_cleanup)`.
- Register console control handler via `SetConsoleCtrlHandler(console_ctrl_handler, TRUE)` to catch `Ctrl+C`, window close, and break signals.
- Always reset terminal attributes `\033[0m` and flush `stdout`.

### 4. Centralized Score Persistence (`include/scores.h`) & Save States
All arcade games share a unified score repository stored in [arcade_scores.ini](arcade_scores.ini):
- **Isolation**: Each game owns a dedicated INI section (e.g. `[tetris]`, `[minesweeper]`, `[wordle]`, `[blackjack]`).
- **Path Resolution**: `arcade_scores_path()` automatically resolves the repository root across execution contexts (Arcade Hub launcher, standalone game executable in `games/<name>/`, or test runner).
- **API**:
  - `int arcade_score_get(const char *section, const char *key)`: Returns score value or 0.
  - `void arcade_score_set(const char *section, const char *key, int value)`: Reads, updates, and atomically saves INI.
- Scores must be saved on round completion and game exit.
- **In-Progress Save States**: Games supporting session resume (e.g. 2048, Sudoku, Wood Block) must store serialized save files locally within `games/<game_name>/` (e.g. `games/<game_name>/save_<game_name>.dat`) to keep the repository root clean.

### 5. Viewport & Rendering Constraints
- **Target Terminal Environment**: Default user environment is **Windows PowerShell / Windows Terminal with 120 columns × 30 rows** (`(get-host).ui.rawui.WindowSize: Width 120, Height 30`).
- **Vertical Line Budget**:
  - Total rendered lines must be strictly `<= 27 lines` (preserving at least 3 lines of padding within a 30-line terminal to guarantee zero scrolling, screen jitter, or rolling).
  - Compact games (Tetris, 2048, Snake, Wordle, Blackjack) remain `<= 21 lines`.
  - Taller multi-card or cascade games (e.g. Klondike Solitaire) utilize up to **27 lines** (16 tableau card rows) so full 13-card cascades (King to Ace) can be displayed without artificial compression.
- **Horizontal Width Budget**:
  - Maximum total line length must be strictly `<= 115 columns` to fit safely within the 120-column terminal without wrapping.
  - Standard compact games use 75-80 columns; wider board games (e.g. Solitaire at 95 columns) expand to provide comfortable card spacing (9-column cards).
- **ANSI Length Calculation**: Never use `strlen()` to pad formatted lines containing ANSI escape sequences or UTF-8 glyphs. Implement `visible_length()` to skip `\033...m` escape codes and UTF-8 continuation bytes (`(c & 0xC0) == 0x80`).

### 6. Documentation Standards
- **No Emojis or Icons**: Documentation files (`README.md`, etc.) must strictly avoid emojis, icons, or decorative glyphs in headers and body text to maintain a clean, professional, minimal style.
- **In-Game Rendering Permitted**: Game logic and in-game ANSI/Unicode UI rendering (boards, cards, pieces, badges) may continue to use symbols, glyphs, and emojis as appropriate for gameplay.
- **Relative Paths Only**: No absolute paths (`C:\...`, `/home/...`, `file:///...`) in committed document files; always use relative paths from the repository root.

---

## Adding a New Game to the Arcade (Checklist)

1. **Create Game Subdirectory**:
   `games/<game_name>/` containing `include/`, `src/`, `test_<game_name>.c`, `Makefile`, and `README.md`.
2. **Implement Standalone Makefile**:
   Must provide `all` (`<game_name>.exe`), `test` (`test_<game_name>.exe`), and `clean` targets. Include `-Iinclude -I../../include`.
3. **Register in Master [Makefile](Makefile)**:
   Add game target to `all`, `test`, `clean`, and `.PHONY`.
4. **Register in Arcade Hub ([src/main.c](src/main.c))**:
   Append an entry to `GAMES[]` table with title, description, binary path, score section, and score key.
5. **Score Persistence**:
   Read and update high scores via `arcade_score_get()` and `arcade_score_set()` from [include/scores.h](include/scores.h).
6. **Documentation**:
   Add user-facing game description, controls, and features to root [README.md](README.md) and `games/<game_name>/README.md`, following documentation formatting standards (plain text headers, no icons or emojis, relative paths only).

---

## Build Pipeline Reference

- **Build Arcade Hub & All Games**:
  ```bash
  make
  ```
  Produces `arcade.exe` launcher and all game executables in `games/<game>/`.

- **Run Full Automated Test Suite**:
  ```bash
  make test
  ```
  Executes unit tests across all games in sequence.

- **Build Individual Games**:
  ```bash
  make tetris
  make 2048
  make minesweeper
  make snake
  make invaders
  make sudoku
  make breaker
  make block
  make pong
  make typing
  make wordle
  make flappy
  make blackjack
  make solitaire
  make chess
  make jigsaw
  make life
  make gyro
  ```

- **Run Individual Game Tests**:
  ```bash
  make -C games/<game_name> test
  ```

- **Clean Entire Workspace**:
  ```bash
  make clean
  ```

---

## 3D Graphics & Simulation Engine Architecture (`gyro/`)

Terminal Studio houses the `gyro/` 3D engine and mathematical simulation laboratory:

```
gyro/
├── src/            # Standalone C simulation & rendering engines
├── bin/            # Compiled demo executables (.exe on Windows)
├── Makefile        # Autonomous build system with OpenMP support
├── README.md       # 3D demo catalog, camera controls, and math specifications
└── FUTURE_TASKS.md # Mathematical roadmap for upcoming 3D simulations
```

### Technical Specs & Compiler Flags
- **Compiler Requirements**: GCC with OpenMP support (`-fopenmp`) and standard math library (`-lm`).
- **Optimization**: Compiled with `-O3` for high-throughput frame generation (Z-buffering, raymarching, N-body physics).
- **Build Commands**:
  - `make gyro`: Builds all 9 demos into `gyro/bin/` from the repository root.
  - `make -C gyro [demo]`: Builds an individual demo (e.g. `make -C gyro black_hole`).
  - `make -C gyro clean`: Cleans all compiled executables in `gyro/bin/`.
- **Documentation Standards**: All documentation files in `gyro/` adhere to repository rules (plain text headers, no emojis/icons, relative paths only).

---

## 24-Bit TrueColor ASCII Video Player Architecture (`ascii_video/`)

Terminal Studio integrates a Python 24-bit TrueColor ASCII video playback engine in `ascii_video/`:

```
ascii_video/
├── __init__.py           # Package exports (AsciiConverter, AsciiVideoPlayer, GlyphMode)
├── __main__.py           # CLI invocation entry point (python -m ascii_video)
├── converter.py          # Geometry, luminance, Sobel edges, and ANSI TrueColor serialization
├── fast_serializer.c     # High-throughput C ANSI TrueColor serializer with delta quantization
├── fast_serializer.py    # Ctypes loader for compiled shared library with NumPy fallback
├── pipeline.py           # Multithreaded producer-consumer prefetch pipeline
├── player.py             # OpenCV video capture, non-blocking input, pacing, and frame dropping
├── cli.py                # Command-line interface argument parser
├── sample_generator.py   # Synthetic test video generator for headless CI and demo mode
├── requirements.txt      # Python dependencies (numpy, opencv-python, colorama, pytest)
├── README.md             # Developer documentation and CLI reference
└── tests/                # Automated headless unit and integration test suite
    ├── __init__.py
    ├── test_converter.py
    ├── test_player.py
    ├── test_edge_cases.py
    └── test_performance.py
```

### Module Responsibilities

1. **`ascii_video/converter.py`**:
   - Computes target character dimensions factoring in non-square monospace font cell aspect ratio (~0.5) and terminal boundary safety (`max_w = term_w - 1`).
   - Calculates perceptual luminance via ITU-R BT.601 standard: Y = 0.299R + 0.587G + 0.114B.
   - Implements interactive contrast scaling centered around luminance 128.0.
   - Computes directional Sobel gradients (Gx, Gy) and discretizes angles into directional glyphs (`-`, `|`, `/`, `\`).
   - Supports three glyph modes: `density` (ramp), `edge` (Sobel contours), and `hybrid` (edge glyphs over density ramp).
   - Delegates TrueColor serialization to `fast_serializer` with configurable color delta quantization.

2. **`ascii_video/fast_serializer.c` & `fast_serializer.py`**:
   - High-throughput C implementation of TrueColor ANSI serialization compiled via GCC to shared library (`.dll` on Windows, `.so` on POSIX).
   - Fast integer formatting and UCS-4 to UTF-8 multi-byte encoding.
   - Color delta quantization / thresholding to eliminate micro-noise in consecutive RGB values, compressing ANSI stream size by 40% to 80% and reducing terminal DirectX/GPU rendering overhead.
   - Seamless automatic fallback to optimized NumPy implementation if the compiled binary is absent.

3. **`ascii_video/pipeline.py`**:
   - Decoupled multithreaded producer-consumer pipeline: background reader thread decodes frames while concurrent worker threads convert frames across available CPU cores.
   - Strictly ordered frame sequencing buffer with generation tracking and thread-safe capture synchronization (`_cap_lock`) for instant, hang-free flushes on seek or resize.
   - Persistent reader thread lifecycle waiting on EOF for downstream rewinds and looping.
   - In-buffer frame dropping: eliminates reader grab starvation storms on high-resolution 4K video by discarding completed lagging frames directly from the prefetch buffer.

4. **`ascii_video/player.py`**:
   - Clock-synchronized pacing strictly tied to wall-clock time (time.perf_counter()), automatically dropping lagging buffer frames to guarantee exact 1.0x real-world playback speed without slow motion.
   - Initial frame warmup: decodes and renders frame 0 immediately upon startup to guarantee zero blank-screen delay.
   - Periodic wall-clock drift resynchronization preventing infinite frame drops on demanding 4K/60 FPS streams.
   - Path sanitization cleanly stripping trailing slashes or backslashes from CLI arguments.
   - Seamless loop wrapping (`loop=True`) with monotonic stream frame indexing and modulo display.
   - Manages interactive playback lifecycle, status bar formatting, dynamic terminal resize polling (`shutil.get_terminal_size()`), and non-blocking keyboard decoding (`msvcrt` on Windows with POSIX fallback).
   - Instant visual feedback during pause: re-renders frame immediately on mode change, contrast change, seek, or terminal resize.

5. **`ascii_video/cli.py`**:
   - CLI parser supporting file paths, flags, `--demo` mode, dimension overrides (`-w`, `-H`), contrast (`-c`), color threshold (`--color-threshold`), frame dropping (`--no-drop-frames`), worker count (`--workers`), edge thresholds, custom ramps, monochrome (`--no-color`), looping (`-l`), and FPS overrides (`--fps`).
   - Automatically searches for `VID_20261022_230029_524.mp4` in current working directory and repository root when no file path is specified.

6. **`ascii_video/sample_generator.py`**:
   - Generates synthetic test video files containing moving color waves, rectangular edges, rotating diagonal lines, and text overlays for verification and demo mode.

### Build and Test Instructions

- Run all unit and integration tests:
  ```bash
  pytest ascii_video/tests
  ```
  Or via Makefile:
  ```bash
  make test-ascii-video
  ```

- Launch ASCII video player:
  ```bash
  python -m ascii_video
  ```
  Or via Makefile:
  ```bash
  make ascii-video
  ```

### Checklist for ASCII Video Module

- [x] Python package structure established in `ascii_video/`
- [x] Core ASCII converter with ITU-R BT.601 luminance and Sobel edge detection (`ascii_video/converter.py`)
- [x] Monospace character cell aspect ratio compensation (~0.5) and terminal boundary safety
- [x] C-accelerated 24-bit TrueColor ANSI serialization with GCC shared library compilation (`ascii_video/fast_serializer.c`)
- [x] Seamless pure Python / NumPy fallback when compiled binary is absent (`ascii_video/fast_serializer.py`)
- [x] Color delta quantization / thresholding compressing ANSI stream size by 40-80%
- [x] Multithreaded producer-consumer prefetch pipeline decoupling decode and conversion across CPU cores (`ascii_video/pipeline.py`)
- [x] Clock-synchronized pacing with automatic frame dropping eliminating slow-motion playback (`ascii_video/player.py`)
- [x] Non-blocking interactive keyboard controls with instant redraw on pause
- [x] Dynamic terminal resize adaptation via `shutil.get_terminal_size()`
- [x] Command-line interface with options for workers, color threshold, and frame dropping (`ascii_video/cli.py`)
- [x] Synthetic sample generator for headless environments and demonstration (`ascii_video/sample_generator.py`)
- [x] Automated unit, integration, and performance test suite (53/53 tests passing)
- [x] Packaging configuration (`pyproject.toml`, `ascii_video/requirements.txt`)
- [x] Root Makefile integration (`ascii-video`, `test-ascii-video`, fast_serializer build target, hooked into `make test`)
- [x] Documentation adhering to plain markdown and relative paths (`ascii_video/README.md`, `README.md`, `AGENTS.md`)
- [x] `.gitignore` updated for Python build artifacts, test media, `*.dll`, `*.so`, and `AGENTS.md`


