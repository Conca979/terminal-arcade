# Cyberpunk Snake (Terminal Edition in C)

A high-speed, retro-futuristic terminal Snake game written in C for Windows console environments featuring neon light-cycle color gradients, dynamic power-up chips, and 60 FPS flicker-free ANSI rendering.

---

## Features

- **Neon Synthwave Aesthetics**: Dynamic light-cycle color gradient from electric cyan to hot magenta along the snake body.
- **Power-Up Chips & Cyber Cores**:
  - `◆ ` **Data Packet**: Regular food (+10 pts, normal growth).
  - `★ ` **Cyber Core**: Timed bonus core (+50 pts, +2 length).
  - `⚡` **Overclock Chip**: Double score multiplier (2x) for 7 seconds.
  - `◎ ` **Phase Shift**: Quantum shift allowing phasing through walls for 7 seconds.
- **Dynamic Speed Acceleration**: Movement tick smoothly scales down as length increases.
- **Arena Modes**: Toggle between Lethal Solid Walls and Toroidal Wrap-Around (`M` key).
- **Persistent High Scores**: Automatically saved to and loaded from `snake_highscore.txt`.
- **Multi-Control Support**: Arrow keys, WASD, and Vim (`HJKL`) bindings.

---

## How to Build & Run

### Using Make

```bash
cd games/snake
make
.\snake.exe
```

### Direct GCC Compilation

```bash
gcc -Wall -Wextra -O2 -Iinclude -o snake.exe src/main.c src/game.c src/input.c src/platform.c src/render.c
.\snake.exe
```

---

## Controls

| Key | Alternative | Action |
| :--- | :--- | :--- |
| `↑` | `W` / `K` | Steer Up |
| `↓` | `S` / `J` | Steer Down |
| `←` | `A` / `H` | Steer Left |
| `→` | `D` / `L` | Steer Right |
| `M` | — | Toggle Wrap Mode / Solid Walls |
| `P` | `Esc` | Pause / Resume |
| `R` | — | Restart Game |
| `Q` | — | Quit Game |

---

## Running Automated Tests

```bash
make test
```
Validates snake initialization, movement, growth, wall wrap modes, and power-up score multipliers.
