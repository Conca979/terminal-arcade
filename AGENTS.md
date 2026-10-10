# Terminal Arcade in C

A collection of high-performance, modular terminal games written in C for Windows console environments with 60 FPS flicker-free ANSI color rendering.

## Architecture & Extensible Project Layout

```
terminal-arcade/
├── src/
│   └── main.c                  # Master Arcade interactive terminal launcher
├── games/
│   ├── tetris/                 # Classic Guideline Tetris with SRS & 7-bag
│   │   ├── include/            # config.h, game.h, input.h, platform.h, render.h
│   │   ├── src/                # game.c, input.c, main.c, platform.c, render.c
│   │   ├── test_tetris.c       # Unit test suite
│   │   ├── Makefile            # Standalone build pipeline
│   │   └── README.md           # Game guide
│   ├── 2048/                   # 4x4 sliding tile puzzle with 32-step undo
│   │   ├── include/            # config.h, game.h, input.h, platform.h, render.h
│   │   ├── src/                # game.c, input.c, main.c, platform.c, render.c
│   │   ├── test_2048.c         # Unit test suite
│   │   ├── Makefile            # Standalone build pipeline
│   │   └── README.md           # Game guide
│   └── minesweeper/            # First-click safe sweeper with chords & timer
│       ├── include/            # config.h, game.h, input.h, platform.h, render.h
│       ├── src/                # game.c, input.c, main.c, platform.c, render.c
│       ├── test_minesweeper.c  # Unit test suite
│       ├── Makefile            # Standalone build pipeline
│       └── README.md           # Game guide
├── Makefile                    # Master build & test pipeline
├── README.md                   # Repository guide and controls
└── AGENTS.md                   # Architecture & developer reference
```

---

## Adding a New Game to the Arcade (Extensibility Guide)

To add any new game (e.g. `snake`, `space-invaders`, `pong`):

1. **Create the Game Directory**:
   `games/<game_name>/` containing:
   - `include/`: Game definitions, structures, and headers.
   - `src/`: Game logic, input handling, rendering, and entry loop.
   - `Makefile`: Standalone build pipeline with `all`, `test`, and `clean` targets.
   - `test_<game_name>.c`: Automated unit tests.

2. **Register in Master [Makefile](file:///c:/Users/admin/projects/personal/tetris/Makefile)**:
   Add a target calling `$(MAKE) -C games/<game_name>` in `all`, `test`, and `clean`.

3. **Register in Arcade Hub ([src/main.c](file:///c:/Users/admin/projects/personal/tetris/src/main.c))**:
   Append a new entry to the `GAMES[]` table with its title, description, and executable path.

---

## Build Pipeline Reference

- **Build Arcade Hub & All Games**:
  ```bash
  make
  ```
  Produces `arcade.exe`, `games/tetris/tetris.exe`, `games/2048/2048.exe`, and `games/minesweeper/minesweeper.exe`.

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
  ```

- **Clean Entire Workspace**:
  ```bash
  make clean
  ```

---

## Controls Reference

### Arcade Hub Launcher (`arcade.exe`)
- `↑` / `↓` / `W` / `S`: Navigate game selection
- `Enter` / `Space`: Launch selected game
- `1` - `3`: Quick select and launch
- `Q`: Quit arcade

### Tetris (`games/tetris/tetris.exe`)
- `←` / `→` or `A` / `D`: Move left / right
- `↑` / `W` / `X`: Rotate clockwise (SRS)
- `Z`: Rotate counter-clockwise
- `↓` / `S`: Soft drop
- `Space`: Hard drop
- `C` / `H` / `0`: Hold piece
- `P` / `Esc`: Pause / resume
- `R`: Restart | `Q`: Quit

### 2048 (`games/2048/2048.exe`)
- `↑` / `↓` / `←` / `→` or `W` / `S` / `A` / `D` or `H` / `J` / `K` / `L`: Slide tiles
- `U` / `Z`: Undo move (up to 32 moves)
- `C` / `Enter`: Continue playing past 2048
- `R`: Restart | `Q`: Quit

### Minesweeper (`games/minesweeper/minesweeper.exe`)
- `Arrows` / `WASD` / `HJKL`: Move cursor
- `Space` / `Enter`: Reveal cell (first-click safe)
- `F`: Toggle flag
- `C`: Chord reveal around completed number
- `1` / `2` / `3`: Switch difficulty (Beginner, Intermediate, Expert)
- `R`: Restart | `Q`: Quit
