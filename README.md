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
```

---

## Running Automated Tests

To run unit tests across all games in the suite:

```bash
make test
```

---

## Adding New Games (Extensibility)

The project is structured so any new C terminal game can be added in 3 steps:

1. Add your game directory in `games/<name>/` with its own `include/`, `src/`, and `Makefile`.
2. Add a build rule to the root `Makefile` (`$(MAKE) -C games/<name>`).
3. Add a line to the `GAMES[]` menu table in `src/main.c`.

---

## License

This project is open source and available under the [MIT License](LICENSE).
