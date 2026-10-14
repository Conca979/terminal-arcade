# Flappy Bird CLI (Terminal Edition in C)

A high-performance, authentic retro implementation of **Flappy Bird** written in C for Windows console environments with 60 FPS flicker-free TrueColor ANSI rendering.

---

## Features

- **Authentic Flight Physics**:
  - Realistic gravity acceleration and flap impulse with velocity clamping and ceiling collisions.
  - Floating idle bobbing animation in Ready state.
  - Dynamic bird rotation sprites matching upward flapping, gliding, and downward diving angles.
- **Procedural Obstacle Pipes**:
  - Endless scrolling pipes with randomized gap heights and positions.
  - Neon green TrueColor pipes with stylized top and bottom cap borders.
  - Precise bounding-box collision detection for pixel-perfect obstacle avoidance.
- **Living World with Parallax Clouds**:
  - Multi-layer cloud scenery drifting across twilight skies at parallax speeds.
  - Scrolling ground soil texture with dual-tone dirt particles.
- **Medal Rewards & High Score Persistence**:
  - Achieve milestone medals based on pipes cleared:
    - 🥉 **Bronze**: 10+ Pipes
    - 🥈 **Silver**: 20+ Pipes
    - 🥇 **Gold**: 30+ Pipes
    - 💎 **Platinum**: 40+ Pipes
  - High scores automatically persist across arcade launches in centralized `arcade_scores.ini`.
- **Auto-Save & Safe Resume**:
  - Progress auto-saves on exit (`savegame.dat`) and resumes safely in Paused mode to prevent instant collisions.
- **Zero Viewport Jitter**:
  - Compact 21-line rendering viewport ensures flawless display on standard 80×24 and 80×25 terminals with zero vertical scrolling.

---

## Controls

| Key | Alternative | Action |
| :--- | :--- | :--- |
| `Space` | `W` / `↑` / `Enter` | Flap Wings / Start Game / Play Again |
| `P` | `Esc` | Pause / Resume Game |
| `R` | — | Restart Fresh Run |
| `Q` | `Esc` | Auto-Save & Return to Arcade Hub |

---

## How to Build & Run

### Using Make

```bash
cd games/flappy
make
.\flappy.exe
```

### Direct GCC Compilation

```bash
gcc -Wall -Wextra -O2 -Iinclude -o flappy.exe src/main.c src/game.c src/input.c src/platform.c src/render.c -lm
.\flappy.exe
```

---

## Running Automated Tests

```bash
make test
```

Executes 9 automated unit tests verifying game initialization, flap impulses, gravity acceleration, ceiling clamping, ground and pipe collisions, score and medal progression, pause/restart, and binary auto-save persistence.
