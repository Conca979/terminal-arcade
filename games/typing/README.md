# Typing Defense CLI (Cyber Citadel in C)

A high-performance, fast-paced arcade typing defense game written in C for Windows console environments with 60 FPS flicker-free ANSI color rendering.

Defend your orbital cyber citadel against descending enemy missiles by typing their English words accurately and rapidly!

---

## Features

- **Massive Word Bank**:
  - Dynamically loads 5,800+ English words from `words.txt` with embedded fallback vocabulary.
  - Progressive word length scaling (shorter 3–5 letter words in early waves, ramping up to complex tactical terms).
- **Multiple Enemy Types & Tactical Threats**:
  - **Standard Missile (`▼`)**: Balanced descent speed and points.
  - **Hypersonic Missile (`▲`)**: Rapid descent; high-priority threat.
  - **Armored Tank Missile (`♦`)**: Heavy armor; defeating it restores +1 Shield HP to your Citadel!
  - **EMP Shock Bomb (`⚡`)**: Detonates upon completion, triggering a screen-clearing EMP blast that wipes out all other descending missiles.
- **Smart Target Locking**:
  - Typing the first letter of any incoming threat automatically locks your Citadel laser cannon onto the closest danger.
  - Visual reticle and dual-color progression: completed letters glow in vibrant neon green while remaining characters display in crisp white.
  - Press `Backspace` at any time to switch targets.
- **Real-Time WPM & Accuracy Metrics**:
  - Live Words-Per-Minute (WPM) calculation.
  - Precision keystroke accuracy percentage tracking.
  - Dynamic combo streak multipliers (`x1`, `x2`, `x3`...) rewarding error-free typing streaks.
- **Centralized High Scores**:
  - High score and maximum WPM persisted to `arcade_scores.ini` under `[typing]`.
- **Auto-Save & Seamless Resuming**:
  - Active waves, score, shield integrity, combo, and active targets automatically persist to `games/typing/savegame.dat`. Resumes instantly upon launching.

---

## Controls

| Key | Alternative | Action |
| :--- | :--- | :--- |
| `A` – `Z` | — | Type letters to target and destroy incoming missiles |
| `Backspace` | `Del` | Unlock / deselect current target |
| `Space` | `Enter` | Skip wave announcement / Restart match on Game Over |
| `P` | `Esc` | Pause / Resume Mission |
| `R` | — | Start Fresh Defense Mission |
| `Q` | `Esc` | Auto-save and Return to Arcade Hub |

---

## How to Build & Run

### Using Make

```bash
cd games/typing
make
.\typing.exe
```

### Direct GCC Compilation

```bash
gcc -Wall -Wextra -O2 -Iinclude -o typing.exe src/main.c src/game.c src/input.c src/platform.c src/render.c
.\typing.exe
```

---

## Running Automated Tests

```bash
make test
```

Validates word bank loading from `words.txt`, target locking, typing progression, combo handling, Citadel shield damage, real-time WPM calculation, accuracy formulas, and state persistence.
