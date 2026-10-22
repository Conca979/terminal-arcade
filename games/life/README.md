# 🧬 Conway's Game of Life CLI

A high-performance, interactive implementation of John Conway's famous cellular automaton in C with 60 FPS flicker-free ANSI color rendering, cell age heat palettes, preset stamping, dual mouse/keyboard controls, and deep history stepping.

---

## Conway's B3/S23 Rules

In each generation, cells transition according to 4 fundamental rules:
1. **Underpopulation**: Any live cell with fewer than 2 live neighbors dies.
2. **Survival**: Any live cell with 2 or 3 live neighbors lives on to the next generation.
3. **Overpopulation**: Any live cell with more than 3 live neighbors dies.
4. **Reproduction**: Any dead cell with exactly 3 live neighbors becomes a live cell.

---

## Features

- **540-Cell Split Arena (30×18)**: 60 character columns × 18 rows, perfectly sized for 120×30 terminals.
- **Dynamic Cell Age Heat Palette**:
  - `Age 1` (Newborn): Neon Cyan
  - `Age 2-4` (Young): Vibrant Emerald Lime
  - `Age 5-14` (Mature): Warm Golden Amber
  - `Age 15+` (Ancient / Stable): Electric Hot Magenta / Violet
- **10 Famous Pattern Presets (`P` & `X`)**:
  - **Spaceships**: Glider, Lightweight Spaceship (LWSS)
  - **Oscillators**: Blinker (p2), Toad (p2), Beacon (p2), Pulsar (p3), Pentadecathlon (p15)
  - **Methuselahs**: R-pentomino, Acorn (survives 5,200+ generations!)
  - **Still Lifes**: Beehive & Loaf
- **Boundary Conditions (`M` key)**:
  - **Toroidal Wrap**: Infinite wraparound on all four edges.
  - **Bounded Walls**: Finite closed boundary grid.
- **Interactive Editing & Time Travel**:
  - Point-and-click mouse drawing & erasing.
  - Step forward (`.`) or step backward in time (`U` / `,` 128-step undo).
  - Variable simulation speed (1 to 60 ticks per second).

---

## Controls

| Action | Keyboard | Mouse |
| :--- | :--- | :--- |
| **Run / Pause** | `Space` | — |
| **Step 1 Gen Forward** | `.` / `N` | — |
| **Step 1 Gen Backward** | `,` / `U` / `Z` | — |
| **Toggle / Draw Cell** | `Enter` / `E` | **Left Click** on cell |
| **Erase Cell** | — | **Right Click** on cell |
| **Move Cursor** | `WASD` / `Arrow Keys` / `HJKL` | Move mouse over arena |
| **Cycle Preset** | `P` / `Tab` | — |
| **Stamp Selected Preset** | `X` / `B` / `S` | — |
| **Toggle Wrap / Bounded** | `M` | — |
| **Adjust Speed** | `+` (faster) / `-` (slower) | — |
| **Clear Grid** | `C` | — |
| **Random Seed** | `R` (25% density) | — |
| **Save Stats & Quit** | `Q` / `ESC` | — |

---

## Build & Run

From repository root:
```bash
make life
.\games\life\life.exe
```

Or standalone:
```bash
cd games/life
make
.\life.exe
```

Run automated tests:
```bash
make test
```
