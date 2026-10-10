# Tetris CLI in C

A modern, guideline-compliant terminal Tetris game written in C for Windows console environments with ANSI color rendering.

## Architecture & Module Boundaries

The project is structured into modular components:

- **[include/config.h](file:///c:/Users/admin/projects/personal/tetris/include/config.h)**: Global constants, tetromino enumerations ([TetrominoType](file:///c:/Users/admin/projects/personal/tetris/include/config.h#L17-L27)), playfield dimensions, state enumerations ([PlayState](file:///c:/Users/admin/projects/personal/tetris/include/config.h#L29-L36)), and action definitions ([GameAction](file:///c:/Users/admin/projects/personal/tetris/include/config.h#L38-L50)).
- **[include/platform.h](file:///c:/Users/admin/projects/personal/tetris/include/platform.h)** / **[src/platform.c](file:///c:/Users/admin/projects/personal/tetris/src/platform.c)**: Windows console initialization, UTF-8 code page setup ([platform_init](file:///c:/Users/admin/projects/personal/tetris/src/platform.c#L24-L58)), virtual terminal sequence processing, high-resolution frame timer ([platform_get_time_ms](file:///c:/Users/admin/projects/personal/tetris/src/platform.c#L80-L85)), and clean signal/exit restoration ([platform_cleanup](file:///c:/Users/admin/projects/personal/tetris/src/platform.c#L60-L78)).
- **[include/input.h](file:///c:/Users/admin/projects/personal/tetris/include/input.h)** / **[src/input.c](file:///c:/Users/admin/projects/personal/tetris/src/input.c)**: Non-blocking console input handler ([input_get_action](file:///c:/Users/admin/projects/personal/tetris/src/input.c#L4-L63)) mapping keyboard arrow keys, WASD, Space, Hold, Pause, and Quit controls.
- **[include/game.h](file:///c:/Users/admin/projects/personal/tetris/include/game.h)** / **[src/game.c](file:///c:/Users/admin/projects/personal/tetris/src/game.c)**: Core game logic, state machine ([GameState](file:///c:/Users/admin/projects/personal/tetris/include/game.h#L18-L47)), Super Rotation System (SRS) wall kicks, 7-bag randomizer, lock delay, collision checking ([game_check_collision](file:///c:/Users/admin/projects/personal/tetris/src/game.c#L183-L203)), ghost piece calculations ([game_get_ghost_y](file:///c:/Users/admin/projects/personal/tetris/src/game.c#L205-L211)), scoring, and high score persistence ([game_save_high_score](file:///c:/Users/admin/projects/personal/tetris/src/game.c#L290-L296)).
- **[include/render.h](file:///c:/Users/admin/projects/personal/tetris/include/render.h)** / **[src/render.c](file:///c:/Users/admin/projects/personal/tetris/src/render.c)**: Double-buffered, flicker-free terminal renderer ([render_frame](file:///c:/Users/admin/projects/personal/tetris/src/render.c#L110-L315)) displaying playfield, ghost piece, hold piece box, next queue preview (3 pieces), score statistics, control hints, and pause/game over overlays.
- **[src/main.c](file:///c:/Users/admin/projects/personal/tetris/src/main.c)**: Game entry point and 60 FPS pacing game loop ([main](file:///c:/Users/admin/projects/personal/tetris/src/main.c#L8-L43)).

## Build Pipeline

The project uses GCC with C99/C11 standards and `make`.

- **Build Game**:
  ```bash
  make
  ```
  Produces `tetris.exe`.

- **Run Automated Test Suite**:
  ```bash
  make test
  ```
  Compiles and executes [test_tetris.c](file:///c:/Users/admin/projects/personal/tetris/test_tetris.c) to validate collision, SRS movement, drops, hold mechanic, and line clear scoring.

- **Clean Build Artifacts**:
  ```bash
  make clean
  ```

## Controls

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
