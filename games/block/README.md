# Classic Wood Block Puzzle CLI

A high-performance terminal recreation of the beloved mobile Wood Block Puzzle on a classic 10×10 grid, featuring warm wooden ANSI aesthetics, real-time mouse picking & placement, ghost preview indicators, endless combo scoring, and persistent auto-resuming.

---

## Features

- **Classic 10×10 Grid Layout**:
  - Faithfully reproduces the standard 10×10 board with side-by-side status statistics and candidate piece trays.
  - Warm natural wooden color palette (Oak, Cedar, Mahogany, Pine, and Amber blocks).
- **Dual Mouse & Keyboard Controls**:
  - **Native Mouse Support**: Click any of the 3 candidate trays to pick up a shape; move your cursor across the grid to inspect placement; left-click on any grid cell to place.
  - **Keyboard Support**: Fast navigation using `WASD` / `Arrows` / `HJKL`, quick piece selection with keys `1`, `2`, `3`, and placement with `Space` or `Enter`.
- **Live Ghost Placement Preview**:
  - Highlights targeted cells in real time:
    - Glowing Emerald `▒` indicates a valid, legal placement.
    - Crimson `✕` alerts you to collisions or out-of-bounds obstacles.
- **Simultaneous Cross & Multi-Line Clears**:
  - Full rows and columns are evaluated simultaneously. Intersecting clears wipe rows and columns together.
  - Generous multi-line bonus scoring (Double: +300, Triple: +600, Mega: +1000+).
  - Back-to-back clear streaks grant progressive `+50 × Streak` multiplier bonuses.
- **25 Balanced Polyomino Shapes**:
  - Dots (1×1), lines (1×2 through 1×5), squares (2×2, 3×3), small corners, large L-shapes, T-shapes, and solid 2×3 blocks.
  - Trays automatically refill whenever all 3 candidate pieces are placed on the board.
- **Endless Mode with Persistent Auto-Save & Resuming**:
  - Board, tray pieces, scores, and streak state are saved to `savegame.dat` after every move.
  - Closing the terminal or pressing `Q` seamlessly persists your run.
  - High scores are saved to `highscore.txt`.

---

## Controls

| Input | Action |
| --- | --- |
| **Mouse Move** | Move cursor / inspect ghost preview on 10×10 grid |
| **Mouse Left-Click** | Pick piece from bottom tray or place onto grid cell |
| `1` / `2` / `3` | Select piece from Tray Slot 1, 2, or 3 |
| `←` `→` `↑` `↓` / `WASD` / `HJKL` | Move board placement cursor |
| `Space` / `Enter` | Place currently selected piece at cursor |
| `Tab` / `0` / `Esc` | Deselect active tray piece |
| `R` | Start fresh endless puzzle run |
| `Q` / `Esc` | Auto-save active run and return to Arcade Hub |

---

## Build & Run

```bash
# Standalone build
make -C games/block

# Launch Wood Block Puzzle
./games/block/block.exe

# Run automated unit test suite
make -C games/block test
```
