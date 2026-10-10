# 2048 CLI (Terminal Edition in C)

A fast, beautifully styled terminal implementation of the classic **2048** puzzle game written in C for Windows console environments with ANSI color styling.

---

## Features

- **Classic 4x4 Gameplay**: 90% spawn chance of 2, 10% spawn chance of 4 on empty cells.
- **Accurate Sliding & Merging**: Tiles slide, merge once per turn, and increment score by merged values.
- **Multi-Level Undo**: Press `U` or `Z` to step backward up to 32 previous moves.
- **Vibrant ANSI Palette**: Unique tailored background and foreground colors for every power of two (2 to 4096+).
- **Persistent High Scores**: High scores are saved to and loaded from `2048_highscore.txt`.
- **Win & Continue**: Reach the 2048 tile to trigger a victory banner, with the option to continue playing for higher numbers.
- **Versatile Controls**: Supports Arrow keys, WASD, and Vim-style HJKL bindings.

---

## How to Build & Run

### Using Make

```bash
cd games/2048
make
.\2048.exe
```

### Direct GCC Compilation

```bash
gcc -Wall -Wextra -O2 -Iinclude -o 2048.exe src/main.c src/game.c src/input.c src/platform.c src/render.c
.\2048.exe
```

---

## Controls

| Key | Alternative | Action |
| :--- | :--- | :--- |
| `↑` | `W` / `K` | Slide Up |
| `↓` | `S` / `J` | Slide Down |
| `←` | `A` / `H` | Slide Left |
| `→` | `D` / `L` | Slide Right |
| `U` | `Z` | Undo Move |
| `C` | `Enter` / `Space` | Continue Playing after 2048 Win |
| `R` | — | Restart Game |
| `Q` | `Esc` | Quit Game |

---

## Running Automated Tests

```bash
make test
```
Validates slide merging, multi-row slides, undo mechanics, win states, and game-over conditions.
