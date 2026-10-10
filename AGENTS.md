# Terminal Arcade in C

A collection of high-performance, guideline-compliant terminal games written in C for Windows console environments with ANSI color rendering.

## Games Collection

1. **Tetris CLI**: Classic guideline Tetris featuring Super Rotation System (SRS), 7-bag randomizer, lock delay, hold piece, 3-piece next queue, and line-clear animations.
2. **2048 CLI**: Tile-sliding mathematical puzzle featuring powers-of-two merges, 32-step undo history, custom ANSI tile color palette, and high score tracking.

---

## Architecture & Module Boundaries

### 1. Tetris CLI (`./`)
- **[include/config.h](file:///c:/Users/admin/projects/personal/tetris/include/config.h)**: Global constants, tetromino enumerations ([TetrominoType](file:///c:/Users/admin/projects/personal/tetris/include/config.h#L17-L27)), playfield dimensions, state enumerations ([PlayState](file:///c:/Users/admin/projects/personal/tetris/include/config.h#L29-L36)), and action definitions ([GameAction](file:///c:/Users/admin/projects/personal/tetris/include/config.h#L38-L50)).
- **[include/platform.h](file:///c:/Users/admin/projects/personal/tetris/include/platform.h)** / **[src/platform.c](file:///c:/Users/admin/projects/personal/tetris/src/platform.c)**: Windows console initialization, UTF-8 code page setup ([platform_init](file:///c:/Users/admin/projects/personal/tetris/src/platform.c#L24-L58)), virtual terminal sequence processing, high-resolution frame timer ([platform_get_time_ms](file:///c:/Users/admin/projects/personal/tetris/src/platform.c#L80-L85)), and clean signal/exit restoration ([platform_cleanup](file:///c:/Users/admin/projects/personal/tetris/src/platform.c#L60-L78)).
- **[include/input.h](file:///c:/Users/admin/projects/personal/tetris/include/input.h)** / **[src/input.c](file:///c:/Users/admin/projects/personal/tetris/src/input.c)**: Non-blocking console input handler ([input_get_action](file:///c:/Users/admin/projects/personal/tetris/src/input.c#L4-L63)) mapping keyboard arrow keys, WASD, Space, Hold, Pause, and Quit controls.
- **[include/game.h](file:///c:/Users/admin/projects/personal/tetris/include/game.h)** / **[src/game.c](file:///c:/Users/admin/projects/personal/tetris/src/game.c)**: Core game logic, state machine ([GameState](file:///c:/Users/admin/projects/personal/tetris/include/game.h#L18-L47)), SRS wall kicks, 7-bag randomizer, lock delay, collision checking ([game_check_collision](file:///c:/Users/admin/projects/personal/tetris/src/game.c#L183-L203)), ghost piece calculations ([game_get_ghost_y](file:///c:/Users/admin/projects/personal/tetris/src/game.c#L205-L211)), scoring, and high score persistence ([game_save_high_score](file:///c:/Users/admin/projects/personal/tetris/src/game.c#L290-L296)).
- **[include/render.h](file:///c:/Users/admin/projects/personal/tetris/include/render.h)** / **[src/render.c](file:///c:/Users/admin/projects/personal/tetris/src/render.c)**: Double-buffered terminal renderer ([render_frame](file:///c:/Users/admin/projects/personal/tetris/src/render.c#L110-L315)) with ghost piece, hold box, next queue, and overlays.
- **[src/main.c](file:///c:/Users/admin/projects/personal/tetris/src/main.c)**: 60 FPS pacing game loop ([main](file:///c:/Users/admin/projects/personal/tetris/src/main.c#L8-L43)).

### 2. 2048 CLI (`games/2048/`)
- **[games/2048/include/config.h](file:///c:/Users/admin/projects/personal/tetris/games/2048/include/config.h)**: Grid dimensions, move directions, action types, and game states.
- **[games/2048/include/game.h](file:///c:/Users/admin/projects/personal/tetris/games/2048/include/game.h)** / **[games/2048/src/game.c](file:///c:/Users/admin/projects/personal/tetris/games/2048/src/game.c)**: Slide merging logic, 32-state undo snapshot stack, spawn randomizer, win/loss detection, and high score persistence.
- **[games/2048/include/input.h](file:///c:/Users/admin/projects/personal/tetris/games/2048/include/input.h)** / **[games/2048/src/input.c](file:///c:/Users/admin/projects/personal/tetris/games/2048/src/input.c)**: Keyboard event handling supporting Arrow keys, WASD, Vim (HJKL), Undo, and Restart.
- **[games/2048/include/platform.h](file:///c:/Users/admin/projects/personal/tetris/games/2048/include/platform.h)** / **[games/2048/src/platform.c](file:///c:/Users/admin/projects/personal/tetris/games/2048/src/platform.c)**: Win32 console VT processing, UTF-8 mode, and signal cleanup.
- **[games/2048/include/render.h](file:///c:/Users/admin/projects/personal/tetris/games/2048/include/render.h)** / **[games/2048/src/render.c](file:///c:/Users/admin/projects/personal/tetris/games/2048/src/render.c)**: Double-buffered 4x4 tile grid with individual color palettes by power-of-two, stats bar, and modals.
- **[games/2048/src/main.c](file:///c:/Users/admin/projects/personal/tetris/games/2048/src/main.c)**: Event-driven game loop.

---

## Build Pipeline

Both games use GCC with `-Wall -Wextra -O2` standards and `make`.

- **Build Tetris**:
  ```bash
  make
  ```
- **Test Tetris**:
  ```bash
  make test
  ```

- **Build 2048**:
  ```bash
  cd games/2048
  make
  ```
- **Test 2048**:
  ```bash
  cd games/2048
  make test
  ```

---

## Controls Reference

### Tetris
| Key | Action |
| --- | --- |
| `←` / `A` | Move Left |
| `→` / `D` | Move Right |
| `↑` / `W` / `X` | Rotate Clockwise |
| `Z` | Rotate Counter-Clockwise |
| `↓` / `S` | Soft Drop |
| `Space` | Hard Drop |
| `C` / `H` / `0` | Hold Piece |
| `P` / `Esc` | Pause / Resume |
| `R` | Restart Game |
| `Q` | Quit |

### 2048
| Key | Alternative | Action |
| --- | --- | --- |
| `↑` | `W` / `K` | Slide Up |
| `↓` | `S` / `J` | Slide Down |
| `←` | `A` / `H` | Slide Left |
| `→` | `D` / `L` | Slide Right |
| `U` | `Z` | Undo Move |
| `C` | `Enter` / `Space` | Continue Playing |
| `R` | — | Restart Game |
| `Q` | `Esc` | Quit |
