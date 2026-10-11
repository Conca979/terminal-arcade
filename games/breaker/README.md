# Neon Balls Bricks Breaker CLI

A fast-paced, high-resolution terminal brick breaker with 2D sub-step ball physics, real-time mouse trajectory aiming, endless round progression, and persistent auto-resuming.

---

## Features

- **High-Resolution Arena (~96 × 42 Canvas)**:
  - 8-column brick grid with 34 vertical flight rows designed for high-density terminals.
  - Numbers clearly displayed on every brick (`[  42 ]` / `[ 128 ]`).
  - Color-coded durability gradient (Cyan/Lime → Amber → Crimson).
- **Native Mouse Point & Click Controls**:
  - Move your mouse to aim the multi-bounce laser guide line in real time.
  - Left-click to release your swarm of balls.
  - Right-click or press `F` for 3x Turbo Fast-Forward speed.
- **Endless Mode Only**:
  - Survive wave after wave as brick durability scales each round.
  - Clear the board before descending bricks cross the danger line (Row 24).
- **Persistent Auto-Save & Resuming**:
  - Every round automatically saves to `savegame.dat`.
  - Closing your terminal or pressing `Q` saves your active run.
  - Launching the game instantly restores your round, swarm size, and arena state.
- **Power-Ups & Tactical Bricks**:
  - `⦿` **Extra Ball Orb**: Expands your ball swarm by `+1` for subsequent rounds.
  - `💣` **Area Bomb**: Detonates in a 3x3 blast when destroyed.
  - `══` / `║║` **Laser Beams**: Fires piercing beams clearing entire rows or columns.

---

## Controls

| Input | Action |
| --- | --- |
| **Mouse Move** | Aim trajectory laser guide in real time |
| **Left Click** / `Space` / `Enter` | Launch ball swarm |
| **Right Click** / `F` / Hold `Space` | Toggle 3x Fast-Forward Turbo mode |
| `←` / `→` or `A` / `D` | Keyboard fine-aim angle adjustment |
| `R` | Start fresh endless run |
| `Q` / `Esc` | Auto-save and exit to Arcade Hub |

---

## Build & Run

```bash
# Standalone build
make -C games/breaker

# Launch Breaker CLI
./games/breaker/breaker.exe

# Run automated tests
make -C games/breaker test
```
