# Terminal Arcade in C

A modular, high-performance collection of classic arcade and puzzle games written in C for Windows console environments with 60 FPS flicker-free ANSI color rendering.

![C Standard](https://img.shields.io/badge/Language-C99%2FC11-blue.svg)
![Platform](https://img.shields.io/badge/Platform-Windows%2010%2F11-0078D6.svg)
![Build](https://img.shields.io/badge/Build-GCC%20%2F%20Make-brightgreen.svg)
![License](https://img.shields.io/badge/License-MIT-green.svg)

---

## Games Included

### 1. 🕹️ Tetris CLI (`games/tetris/`)
Classic Guideline Tetris featuring:
- Super Rotation System (SRS) with full wall and floor kicks
- 7-Bag fair piece randomizer
- Ghost piece projection & Hold piece box
- 3-piece Next Queue preview
- 500 ms lock delay with move/rotate resets
- Visual row-clear flash animations

### 2. 🔢 2048 CLI (`games/2048/`)
Tile-sliding mathematical puzzle featuring:
- 4x4 sliding grid with true power-of-two merge logic
- **32-step Undo History** (`U` key)
- Custom ANSI TrueColor palette for every tile value (2 to 4096+)
- Win banner upon reaching 2048 tile with option to continue playing
- Dual controls: Arrows, WASD, and Vim (`HJKL`)

### 3. 💣 Minesweeper CLI (`games/minesweeper/`)
Full-featured minefield sweeper featuring:
- **First-Click Safety Guarantee** (initial reveal is never a mine)
- Recursive flood-fill for zero-neighbor cells
- Chord reveal (`C` key) to quickly uncover neighbors
- Flagging system (`F` key) with remaining mine counter
- 3 Difficulty presets: Beginner (9x9, 10 mines), Intermediate (16x16, 40 mines), Expert (30x16, 99 mines)
- Live stopwatch timer & best time records

### 4. 🐍 Cyberpunk Snake (`games/snake/`)
Retro-futuristic light-cycle snake featuring:
- Neon Synthwave gradient trail from electric cyan to hot magenta
- **Cyber Food Chips**: Data Packets (`+10`), Timed Cyber Cores (`+50`), Overclock Chips (`2x` score), and Phase Shift (wall walk)
- Smooth dynamic speed acceleration curve as snake grows
- Dual arena navigation: Solid Lethal Walls vs. Toroidal Wrap-Around (`M` key)
- Persistent high score tracking

### 5. 👾 Space Invaders (`games/invaders/`)
Classic arcade alien shooter featuring:
- 32-alien marching swarm with 2-frame retro animations
- Accelerating marching rhythm as the alien swarm thins out
- 4 Destructible defense barricades / bunkers with erosion physics
- Alien bomb drops and mystery mothership (UFO) fly-bys
- Multi-wave progression and persistent high score tracking

### 6. 🧩 Sudoku CLI (`games/sudoku/`)
Logic number puzzle featuring:
- Procedural board generation with unique solutions across 4 difficulty levels
- Candidate pencil notes mode (`N` key) for draft calculations
- Side-by-side Cell Inspector panel with valid candidates and remaining digit counts
- Crosshair and identical number highlights with real-time conflict checking
- 64-step undo stack (`U`) and hint assistant (`H`)

### 7. 🔮 Neon Balls Bricks Breaker (`games/breaker/`)
Physics-based casual arcade puzzle featuring:
- High-resolution arena (~96 × 42) optimized for large console displays
- Real-time mouse trajectory laser aiming with multi-wall bounce preview
- Left-click or Space to fire streaming ball swarms; right-click for 3x Turbo mode
- Endless mode with escalating brick health and danger line threshold
- Persistent auto-save system that seamlessly preserves and resumes runs

### 8. 🪵 Classic Wood Block Puzzle (`games/block/`)
Classic 10×10 wooden tile puzzle featuring:
- High-fidelity 10×10 grid layout with side-by-side stats and 3-piece candidate tray
- Real-time mouse picking & placement with glowing emerald/crimson ghost placement preview
- Dual controls: Point-and-click mouse or keyboard (`1-3` slots, `WASD`/`Arrows`, `Space`/`Enter`)
- Simultaneous horizontal and vertical cross-line clears with progressive streak multipliers
- 25 polyomino shapes with automatic tray refill upon exhaustion
- Endless mode with persistent auto-save and seamless resume

### 9. 🏓 Classic Pong (`games/pong/`)
Retro table tennis arcade game featuring:
- High-speed 60 FPS physics with angle deflection and velocity acceleration
- Player vs. CPU with 3 selectable AI difficulty modes (Easy, Medium, Hard)
- First-to-11 championship match rules with retro segmented scoreboard
- Persistent auto-save system and high scores tracking

### 10. ⌨️ Cyber Citadel Typing Defense (`games/typing/`)
Fast-paced space defense typing shooter featuring:
- 5,800+ English word vocabulary scaled dynamically across waves
- 4 Enemy variants: Normal drones, Fast interceptors, Tank dreadnoughts, and EMP bombs
- Citadel particle explosions, laser cannon targeting, and combo multipliers
- Live Words Per Minute (WPM) and typing accuracy metrics
- Persistent auto-save system and centralized score tracking

---

## Quick Start

### 1. Clone the Repository

```bash
git clone https://github.com/Conca979/terminal-arcade.git
cd terminal-arcade
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
