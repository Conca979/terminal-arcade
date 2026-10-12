# Classic Pong CLI (Terminal Edition in C)

A high-performance, authentic retro arcade implementation of **Pong** (Player vs. Computer) written in C for Windows console environments with 60 FPS flicker-free ANSI rendering.

---

## Features

- **Player vs. Computer AI**:
  - Play against an intelligent computer paddle with 3 difficulty modes:
    - **Easy (Casual)**: Relaxed tracking speed for beginners.
    - **Medium (Arcade)**: Balanced reaction speed for competitive rallies.
    - **Hard (Pro)**: High-speed tracking and precision defensive angles.
  - Switch difficulty on the fly anytime by pressing `Tab` or `M`.
- **Large Retro Digital Scoreboard**:
  - Top-mounted giant 5×4 segment block digits displaying real-time Player and CPU scores directly flanking the center dotted net.
- **Dynamic Physics & Angle Deflection**:
  - Realistic paddle deflection: Hitting the edge of the paddle angles the ball sharply, while center hits send it straight.
  - Progressive rally acceleration: Ball speed increases by 5% on each paddle return up to a fast maximum velocity.
  - Dotted vertical center net and top/bottom court wall reflections.
- **Motion Blur / Speed Trail**:
  - Fading multi-segment particle tail behind the ball matching classic arcade CRT monitor phosphor persistence.
- **Centralized High Scores**:
  - Tracks longest rally streak (`high_score`) and match win records across sessions via `arcade_scores.ini`.
- **Auto-Save & Seamless Resuming**:
  - Mid-game score, paddle positions, rally count, and ball trajectory are auto-saved to `games/pong/savegame.dat`. If you exit or close the terminal, your match resumes instantly.

---

## Controls

| Key | Alternative | Action |
| :--- | :--- | :--- |
| `W` / `S` | `↑` / `↓` or `K` / `J` | Move Player Paddle Up / Down |
| `Space` | `Enter` | Serve Ball / Pause / Resume |
| `Tab` | `M` | Cycle AI Difficulty (Easy → Medium → Hard) |
| `P` | `Esc` | Pause / Resume Game |
| `R` | — | Reset Match (starts fresh game) |
| `Q` | `Esc` | Auto-save Match and Return to Arcade Hub |

---

## How to Build & Run

### Using Make

```bash
cd games/pong
make
.\pong.exe
```

### Direct GCC Compilation

```bash
gcc -Wall -Wextra -O2 -Iinclude -o pong.exe src/main.c src/game.c src/input.c src/platform.c src/render.c
.\pong.exe
```

---

## Running Automated Tests

```bash
make test
```

Validates court initialization, paddle boundary clamping, top/bottom wall reflections, paddle collision and deflection angle calculations, CPU AI tracking, goal scoring, and binary auto-save/resume persistence.
