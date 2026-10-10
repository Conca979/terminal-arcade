# Tetris CLI (Terminal Edition in C)

A modern, guideline-compliant terminal Tetris game written in C for Windows console environments with 60 FPS flicker-free ANSI color rendering.

---

## Features

- **Guideline Mechanics**: 10x20 visible matrix with a 2-row buffer zone.
- **Super Rotation System (SRS)**: Official rotation offsets and wall/floor kick tables for all pieces.
- **7-Bag Randomizer**: Fair tetromino distribution preventing piece droughts.
- **Ghost Piece**: Real-time projection showing exactly where the piece will land.
- **Hold Mechanic**: Hold pieces with instant swap (one hold per drop).
- **Next Queue Preview**: 3 upcoming pieces displayed in dedicated preview panels.
- **Lock Delay**: 500 ms lock delay allowing last-moment slides and rotations.
- **Line Clear Visuals**: Visual flash animations upon row clears.
- **Scoring & Progression**: Traditional scoring multipliers with level gravity curves.
- **High Score Persistence**: Automatically saved to and loaded from `tetris_highscore.txt`.

---

## How to Build & Run

```bash
cd games/tetris
make
.\tetris.exe
```

---

## Controls

| Key | Alternative | Action |
| :--- | :--- | :--- |
| `←` | `A` | Move Left |
| `→` | `D` | Move Right |
| `↑` | `W` / `X` | Rotate Clockwise |
| `Z` | — | Rotate Counter-Clockwise |
| `↓` | `S` | Soft Drop |
| `Space` | — | Hard Drop |
| `C` | `H` / `0` | Hold Piece |
| `P` | `Esc` | Pause / Resume |
| `R` | — | Restart Game |
| `Q` | — | Quit Game |
