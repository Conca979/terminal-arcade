# Minesweeper CLI (Terminal Edition in C)

A classic, full-featured terminal implementation of **Minesweeper** written in C for Windows console environments with ANSI color rendering.

---

## Features

- **First-Click Safety Guarantee**: The initial revealed cell is guaranteed to never contain a mine.
- **Recursive Flood-Fill**: Uncovers all empty adjacent fields and reveals the numbered perimeter.
- **Chord Reveal**: Quick-reveal unflagged neighbors around a completed number tile by pressing `C`.
- **Flagging System**: Place and remove flags (`F`) with real-time mine counter calculation.
- **Multiple Difficulties**:
  - `[1] Beginner`: 9x9 grid, 10 mines
  - `[2] Intermediate`: 16x16 grid, 40 mines
  - `[3] Expert`: 30x16 grid, 99 mines
- **Timer & High Scores**: Elapsed time tracking and high scores saved to `minesweeper_scores.txt`.
- **Zero-Flicker ANSI Rendering**: Smooth double-buffered terminal updates.

---

## How to Build & Run

### Using Make

```bash
cd games/minesweeper
make
.\minesweeper.exe
```

### Direct GCC Compilation

```bash
gcc -Wall -Wextra -O2 -Iinclude -o minesweeper.exe src/main.c src/game.c src/input.c src/platform.c src/render.c
.\minesweeper.exe
```

---

## Controls

| Key | Action |
| :--- | :--- |
| `↑` / `W` / `K` | Move Cursor Up |
| `↓` / `S` / `J` | Move Cursor Down |
| `←` / `A` / `H` | Move Cursor Left |
| `→` / `D` / `L` | Move Cursor Right |
| `Space` / `Enter` | Reveal Cell |
| `F` | Toggle Flag |
| `C` | Chord Reveal |
| `1` / `2` / `3` | Select Difficulty |
| `R` | Restart / Reset Game |
| `Q` / `Esc` | Quit Game |

---

## Running Automated Tests

```bash
make test
```
Tests initialization, first-click safety, recursive reveal, flagging, and difficulty switching.
