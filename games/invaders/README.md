# Space Invaders CLI (Terminal Edition in C)

A classic, action-packed arcade space shooter written in C for Windows console environments with 60 FPS flicker-free ANSI color rendering.

---

## Features

- **Animated Alien Swarm**: 32 aliens across 4 rows with classic 2-frame marching animations.
- **Dynamic Speed-Up**: The remaining swarm accelerates as aliens are eliminated!
- **Destructible Defense Bunkers**: 4 defensive barricades that take localized damage and erode under laser fire.
- **Alien Bomb Attacks**: Periodic bombs dropped from the lowest alive aliens in marching columns.
- **Mystery UFO / Mothership**: Periodically zooms across the top perimeter awarding 100-300 bonus points.
- **Wave Progression**: Defeating all invaders advances to the next wave with faster attack rhythms.
- **High Score Tracking**: Automatically saved to and loaded from `invaders_highscore.txt`.

---

## How to Build & Run

### Using Make

```bash
cd games/invaders
make
.\invaders.exe
```

### Direct GCC Compilation

```bash
gcc -Wall -Wextra -O2 -Iinclude -o invaders.exe src/main.c src/game.c src/input.c src/platform.c src/render.c
.\invaders.exe
```

---

## Controls

| Key | Alternative | Action |
| :--- | :--- | :--- |
| `←` | `A` | Move Cannon Left |
| `→` | `D` | Move Cannon Right |
| `Space` | `W` / `↑` | Fire Laser Cannon |
| `P` | `Esc` | Pause / Resume |
| `R` | — | Restart Game |
| `Q` | — | Quit to Arcade |

---

## Running Automated Tests

```bash
make test
```
Validates cannon movement, laser firing, continuous alien collision detection, scoring, and bunker erosion.
