# Terminal Studio in C

A modular, high-performance collection of classic games, ASCII art, 3D terminal graphics, simulations, and visual demos written in C for Windows console environments with 60 FPS flicker-free ANSI color rendering.

![C Standard](https://img.shields.io/badge/Language-C99%2FC11-blue.svg)
![Platform](https://img.shields.io/badge/Platform-Windows%2010%2F11-0078D6.svg)
![Build](https://img.shields.io/badge/Build-GCC%20%2F%20Make-brightgreen.svg)
![License](https://img.shields.io/badge/License-MIT-green.svg)

---

## Games Included

| Game | Directory | Description | Key Mechanics |
|---|---|---|---|
| **Tetris** | `games/tetris/` | Classic Guideline falling-block puzzle | Super Rotation System (SRS), 7-bag randomizer, ghost projection, hold piece |
| **2048** | `games/2048/` | Tile-sliding power-of-two puzzle | 32-step undo, TrueColor tile palettes, win continuation |
| **Minesweeper** | `games/minesweeper/` | Tactical minefield sweeper | First-click safety, flood-fill, chord reveal, 3 difficulty tiers |
| **Cyberpunk Snake** | `games/snake/` | Retro-futuristic light-cycle snake | Synthwave gradient, cyber food chips, solid wall vs. toroidal wrap modes |
| **Space Invaders** | `games/invaders/` | Classic arcade alien shooter | 32-alien swarm, 4 destructible defense bunkers, mystery UFO bonus passes |
| **Sudoku** | `games/sudoku/` | Procedural logic number puzzle | 4 difficulty tiers, candidate notes, inspector panel, 64-step undo stack |
| **Neon Bricks Breaker** | `games/breaker/` | Physics-based brick breaking arena | Mouse laser trajectory aiming, streaming swarms, 3x turbo mode |
| **Classic Wood Block** | `games/block/` | 10x10 wooden polyomino puzzle | Mouse drag-and-drop, horizontal and vertical cross clears, auto-save |
| **Classic Pong** | `games/pong/` | Retro table tennis simulator | 60 FPS physics, 3 CPU difficulty levels, first-to-11 match rules |
| **Cyber Citadel Typing** | `games/typing/` | Space defense typing shooter | 5,800+ word dictionary, 4 enemy variants, live WPM and accuracy metrics |
| **Wordle CLI** | `games/wordle/` | 5-letter daily word guesser | Official two-pass rules, QWERTY clue tracker, streak statistics |
| **Flappy Bird** | `games/flappy/` | Precision flight obstacle avoider | Gravity impulses, parallax twilight backdrop, 4-tier medal achievements |
| **Blackjack 21** | `games/blackjack/` | Casino table card game | 6-deck shoe, natural 21 (3:2), pair splits, double down, insurance, surrender |
| **Klondike Solitaire** | `games/solitaire/` | Classic patience card game | 7 tableau columns, draw-1 waste recycle, 64-step undo, auto-solve |
| **Chess Master** | `games/chess/` | FIDE rules with Negamax AI | 3 AI difficulty tiers, Quiescence search, split-screen match dashboard, undo |
| **Jigsaw Snap** | `games/jigsaw/` | Pixel art pick-and-place puzzle | 4x4 snap board, 5 artwork themes, hint solver, full preview toggle |
| **Conway's Life** | `games/life/` | Interactive cellular automaton | Dynamic age heat palette, 10 pattern presets, mouse drawing, time travel |

---

## 3D Graphics and Simulations (Gyro)

Located in `gyro/`, Gyro is a zero-dependency 3D terminal graphics engine and physics simulation suite written in pure C:
- **Schwarzschild Black Hole**: Relativistic raymarching simulating gravitational lensing and Doppler beaming with OpenMP (`gyro/src/black_hole.c`).
- **Celestial Mechanics and Orbits**: Real-time multi-body solar system with dynamic hemisphere point lighting (`gyro/src/solar_spinning.c`) and third-person body navigation (`gyro/src/body_moving.c`).
- **Parametric 3D Geometries**: Surface normal luminance torus (`gyro/src/donut_spinning.c`), DNA double helix (`gyro/src/dna_spinning.c`), and depth-sorted triple rotating cubes (`gyro/src/cube_spinning.c`).
- **Interactive 3D Games and Puzzles**: Full 3x3x3 Rubik's cube with 2D scanline rasterization (`gyro/src/rubiks_cube.c`), deductive 3D Cube Sweeper (`gyro/src/cube_sweeper.c`), and 3D Raymarched Duck Hunt (`gyro/src/duck_hunt.c`).

### Building Gyro Demos

```bash
# Build all 3D demos into gyro/bin/
make gyro

# Or build standalone
cd gyro
make
```

All 3D executables are output to `gyro/bin/`. Detailed controls and mathematical specifications are documented in [gyro/README.md](gyro/README.md).

---

## 24-Bit TrueColor ASCII Video Player (`ascii_video/`)

Located in `ascii_video/`, this Python tool converts and streams MP4 video files directly in terminal emulators using 24-bit TrueColor ANSI escape sequences:
- **C-Accelerated Serialization**: Sub-millisecond ANSI TrueColor serialization compiled via GCC to shared library (`.dll`/`.so`) with color delta thresholding and NumPy fallback.
- **Multithreaded Prefetch Pipeline**: Decouples video decoding and frame conversion across multiple CPU cores for high-throughput playback.
- **Clock-Synchronized Pacing**: Ties playback strictly to `time.perf_counter()` with automatic frame dropping to eliminate slow-motion playback on large displays.
- **Dynamic Resolution Scaling**: Continuously queries `shutil.get_terminal_size()` and adapts video grid dimensions in real time when resizing or zooming (Ctrl +/-).
- **Aspect Ratio Compensation**: Monospace character cells (~0.5 width-to-height ratio) are factored into area-downsampling (`cv2.INTER_AREA`) to preserve undistorted video geometry.
- **Perceptual Luminance & Contrast**: Computes ITU-R BT.601 luminance ($Y = 0.299R + 0.587G + 0.114B$) with interactive contrast scaling.
- **Directional Sobel Edges**: Detects spatial gradients and discretizes orientations into directional glyphs (`-`, `|`, `/`, `\`), composited with density ramps in hybrid mode.
- **Zero-Flicker Rendering**: Single in-place frame buffer write (`\x1b[H`) with trailing line clears (`\x1b[K`).
- **Interactive Controls**: Non-blocking keyboard controls for pause/resume (`Space`), glyph mode toggle (`M`), contrast adjust (`+` / `-`), seek (`[` / `]`), restart (`R`), and quit (`Q` / `Esc`).

### Running the ASCII Video Player

```bash
# Play default video (VID_20261022_230029_524.mp4 in repo root)
python -m ascii_video

# Play specific video file
python -m ascii_video path/to/video.mp4

# Run synthetic demonstration
python -m ascii_video --demo
```

Detailed options and developer documentation are located in [ascii_video/README.md](ascii_video/README.md).

---

## Quick Start

### 1. Clone the Repository

```bash
git clone https://github.com/Conca979/terminal-studio.git
cd terminal-studio
```

### 2. Build Everything

Using **Make**:
```bash
make
```

### 3. Launch the Arcade Hub

Run the master interactive terminal launcher:
```powershell
.\arcade.exe
```

Or run any game standalone directly:
```powershell
.\games\tetris\tetris.exe
.\games\2048\2048.exe
.\games\minesweeper\minesweeper.exe
.\games\snake\snake.exe
.\games\invaders\invaders.exe
.\games\sudoku\sudoku.exe
.\games\breaker\breaker.exe
.\games\block\block.exe
.\games\pong\pong.exe
.\games\typing\typing.exe
.\games\wordle\wordle.exe
.\games\flappy\flappy.exe
.\games\blackjack\blackjack.exe
.\games\solitaire\solitaire.exe
.\games\chess\chess.exe
.\games\jigsaw\jigsaw.exe
.\games\life\life.exe
```

---

## Running Automated Tests

To run unit tests across all games in the suite:

```bash
make test
```

---

## Centralized High Scores System

All arcade games share a unified, structured score repository stored in `arcade_scores.ini`:
- **Game Isolation**: Each game owns its dedicated section (e.g., `[tetris]`, `[minesweeper]`, `[breaker]`), preventing any cross-game data overwrite.
- **Dynamic Path Resolution**: `include/scores.h` automatically resolves the project root from any execution context (Arcade Hub, standalone game executable, or unit test runner).
- **Live Hub Display**: The interactive `arcade.exe` launcher queries the store in real time and displays your current best score for each game directly on the menu.

---

## Adding New Games (Extensibility)

The project is structured so any new C terminal game can be added in 3 steps:

1. Add your game directory in `games/<name>/` with its own `include/`, `src/`, and `Makefile`.
2. Add a build rule to the root `Makefile` (`$(MAKE) -C games/<name>`).
3. Add a line to the `GAMES[]` menu table in `src/main.c`.

---

## License

This project is open source and available under the [MIT License](LICENSE).
