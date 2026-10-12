# Terminal Arcade in C

A collection of high-performance, modular terminal games written in C for Windows console environments with 60 FPS flicker-free ANSI color rendering.

## Architecture & Extensible Project Layout

```
terminal-arcade/
├── include/
│   └── scores.h                # Centralized INI score management for all arcade games
├── src/
│   └── main.c                  # Master Arcade interactive terminal launcher with live high scores
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
│   ├── minesweeper/            # First-click safe sweeper with chords & timer
│   │   ├── include/            # config.h, game.h, input.h, platform.h, render.h
│   │   ├── src/                # game.c, input.c, main.c, platform.c, render.c
│   │   ├── test_minesweeper.c  # Unit test suite
│   │   ├── Makefile            # Standalone build pipeline
│   │   └── README.md           # Game guide
│   ├── snake/                  # Cyberpunk light-cycle snake with power-ups & buffs
│   │   ├── include/            # config.h, game.h, input.h, platform.h, render.h
│   │   ├── src/                # game.c, input.c, main.c, platform.c, render.c
│   │   ├── test_snake.c        # Unit test suite
│   │   ├── Makefile            # Standalone build pipeline
│   │   └── README.md           # Game guide
│   ├── invaders/               # Classic Space Invaders with bunkers & UFO
│   │   ├── include/            # config.h, game.h, input.h, platform.h, render.h
│   │   ├── src/                # game.c, input.c, main.c, platform.c, render.c
│   │   ├── test_invaders.c     # Unit test suite
│   │   ├── Makefile            # Standalone build pipeline
│   │   └── README.md           # Game guide
│   ├── sudoku/                 # Logic puzzle with notes, conflicts & inspector
│   │   ├── include/            # config.h, game.h, input.h, platform.h, render.h
│   │   ├── src/                # game.c, input.c, main.c, platform.c, render.c
│   │   ├── test_sudoku.c       # Unit test suite
│   │   ├── Makefile            # Standalone build pipeline
│   │   └── README.md           # Game guide
│   ├── breaker/                # Neon Balls Bricks Breaker with mouse aim & auto-save
│   │   ├── include/            # config.h, game.h, input.h, platform.h, render.h
│   │   ├── src/                # game.c, input.c, main.c, platform.c, render.c
│   │   ├── test_breaker.c      # Unit test suite
│   │   ├── Makefile            # Standalone build pipeline
│   │   └── README.md           # Game guide
│   ├── block/                  # Classic 10x10 Wood Block Puzzle with mouse & auto-save
│   │   ├── include/            # config.h, game.h, input.h, platform.h, render.h
│   │   ├── src/                # game.c, input.c, main.c, platform.c, render.c
│   │   ├── test_block.c        # Unit test suite
│   │   ├── Makefile            # Standalone build pipeline
│   │   └── README.md           # Game guide
│   ├── pong/                   # Classic Pong (Player vs CPU) with retro scoreboard & AI
│   │   ├── include/            # config.h, game.h, input.h, platform.h, render.h
│   │   ├── src/                # game.c, input.c, main.c, platform.c, render.c
│   │   ├── test_pong.c         # Unit test suite
│   │   ├── Makefile            # Standalone build pipeline
│   │   └── README.md           # Game guide
│   └── typing/                 # Cyber Citadel Typing Defense with 5,800+ English words
│       ├── include/            # config.h, game.h, input.h, platform.h, render.h
│       ├── src/                # game.c, input.c, main.c, platform.c, render.c
│       ├── test_typing.c       # Unit test suite
│       ├── Makefile            # Standalone build pipeline
│       └── README.md           # Game guide
├── Makefile                    # Master build & test pipeline
├── README.md                   # Repository guide and controls
└── AGENTS.md                   # Architecture & developer reference
```

---

## Adding a New Game to the Arcade (Extensibility Guide)

To add any new game (e.g. `breakout`, `pong`, `sokoban`):

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
  Produces `arcade.exe`, `games/tetris/tetris.exe`, `games/2048/2048.exe`, `games/minesweeper/minesweeper.exe`, `games/snake/snake.exe`, `games/invaders/invaders.exe`, `games/sudoku/sudoku.exe`, `games/breaker/breaker.exe`, `games/block/block.exe`, `games/pong/pong.exe`, and `games/typing/typing.exe`.

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
- `1` - `9`: Quick select and launch
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
- `R`: Start fresh game (clears active save)
- `Q` / `Esc`: Auto-save board & progress, and return to Arcade Hub

### Minesweeper (`games/minesweeper/minesweeper.exe`)
- `Arrows` / `WASD` / `HJKL`: Move cursor
- `Space` / `Enter`: Reveal cell (first-click safe)
- `F`: Toggle flag
- `C`: Chord reveal around completed number
- `1` / `2` / `3`: Switch difficulty (Beginner, Intermediate, Expert)
- `R`: Restart | `Q`: Quit

### Cyberpunk Snake (`games/snake/snake.exe`)
- `Arrows` / `WASD` / `HJKL`: Steer snake
- `M`: Toggle Wrap Mode / Solid Walls
- `P` / `Esc`: Pause / resume
- `R`: Restart | `Q`: Quit

### Space Invaders (`games/invaders/invaders.exe`)
- `←` / `→` or `A` / `D`: Move Cannon
- `Space` / `W` / `↑`: Fire Laser
- `P` / `Esc`: Pause / resume
- `R`: Restart | `Q`: Quit

### Sudoku (`games/sudoku/sudoku.exe`)
- `Arrows` / `WASD` / `HJKL`: Move cursor across 9x9 grid
- `1` - `9`: Place digit / Toggle candidate note
- `N`: Toggle Pencil Notes mode
- `0` / `Backspace` / `Del` / `X`: Clear cell
- `U` / `Z`: Undo move (up to 64 steps)
- `H`: Hint (reveals correct cell value)
- `E`: Toggle conflict checking
- `Tab` / `M`: Cycle difficulty (Easy, Medium, Hard, Expert)
- `P` / `Esc`: Pause / resume
- `R`: Start fresh puzzle (clears active save)
- `Q` / `Esc`: Auto-save in-progress puzzle & timer, and return to Arcade Hub

### Neon Balls Bricks Breaker (`games/breaker/breaker.exe`)
- `Mouse Move`: Aim trajectory laser guide in real time
- `Left Click` / `Space` / `Enter`: Launch ball swarm
- `Right Click` / `F` / Hold `Space`: Toggle 3x Fast-Forward Turbo mode
- `←` / `→` or `A` / `D`: Adjust aim angle via keyboard
- `R`: Start fresh endless run
- `Q` / `Esc`: Auto-save progress and return to Arcade Hub

### Classic Wood Block Puzzle (`games/block/block.exe`)
- `Mouse Move`: Move cursor / view live ghost placement preview
- `Left Click`: Pick piece from bottom tray or place on grid cell
- `1` / `2` / `3`: Select piece from Tray Slot 1, 2, or 3
- `←` `→` `↑` `↓` / `WASD` / `HJKL`: Move placement cursor across 10×10 grid
- `Space` / `Enter`: Place currently selected piece at cursor
- `Tab` / `0` / `Esc`: Deselect active piece
- `R`: Start fresh endless run
- `Q` / `Esc`: Auto-save progress and return to Arcade Hub

### Classic Pong (`games/pong/pong.exe`)
- `W` / `S` or `↑` / `↓`: Move player paddle
- `Space` / `Enter`: Serve ball / Pause / Resume
- `Tab` / `M`: Cycle AI difficulty (Easy, Medium, Hard)
- `R`: Reset match (starts fresh game)
- `Q` / `Esc`: Auto-save match progress and return to Arcade Hub

### Cyber Citadel Typing Defense (`games/typing/typing.exe`)
- `A` – `Z`: Type English words to lock onto and destroy incoming missiles
- `Backspace` / `Del`: Unlock / cancel current locked target
- `Space` / `Enter`: Skip wave announcement / Restart mission on Game Over
- `P` / `Esc`: Pause / resume mission
- `R`: Reset mission (starts fresh game)
- `Q` / `Esc`: Auto-save mission progress and return to Arcade Hub
