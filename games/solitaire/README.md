# Klondike Solitaire CLI (Terminal Classic Edition in C)

A high-performance, authentic implementation of **Klondike Solitaire** written in C for Windows console environments with 60 FPS flicker-free TrueColor ANSI rendering.

---

## Features

- **Authentic Klondike Rules**:
  - **Standard 52-Card Deck**: Four suits (♠ ♥ ♦ ♣) shuffled using Fisher-Yates randomization.
  - **7-Column Tableau Cascade**: Columns 1 through 7 with standard descending alternating color sequences (Red on Black, Black on Red).
  - **King-to-Empty-Space**: Empty tableau columns accept Kings or sequences led by a King.
  - **Automatic Card Reveal**: Uncovering face-down cards turns them face up automatically with a +5 score reward.
  - **4 Foundation Piles**: Build each suit from Ace up through King (+10 score per card).
  - **Stock & Waste**: Draw cards one-by-one from stock; recycle waste into stock when exhausted (-20 pts).
- **Dual Input Modes**:
  - **Quick Action Keys**: Press `S` to draw, `W` for waste, `1-7` for tableau columns, `F` for foundation, `A` for auto-foundation, and `U` for undo.
  - **Full Cursor Navigation**: Move cursor with Arrow keys, WASD, or Vim keys (`HJKL`) and press `Enter`/`Space` to pick and drop.
  - **Point-and-Click Mouse Support**: Left-click to draw, select, or place cards; right-click any card to instantly send it to an eligible Foundation!
- **Assistance & Comfort**:
  - **Deep 64-Step Undo Stack (`U`)**: Seamlessly revert draws, moves, and auto-placements.
  - **Smart Hint Assistant (`H`)**: Highlights the optimal available move or tells you when to draw.
  - **Auto-Foundation (`A`)**: Scans tableau and waste to automatically move eligible cards (Aces and low cards) to foundations.
- **Strict 21-Line Viewport Budget**:
  - Compact adaptive stacking and rendering ensures 100% flicker-free output with zero vertical scrolling jitter on standard 80×24 and 80×25 terminals.
- **Centralized Score Persistence**:
  - Tracks high score, games played, and games won in `arcade_scores.ini`.

---

## Controls

| Key | Action |
| :--- | :--- |
| `S` | **Draw from Stock** (or recycle waste when stock is empty) |
| `W` | **Select Waste card** |
| `1` - `7` | **Select / Target Tableau Column 1 to 7** |
| `F` | **Target Foundation** (or Auto-Move if nothing selected) |
| `A` | **Auto-Move**: Automatically send eligible cards to Foundation |
| `U` | **Undo**: Revert the last move |
| `H` | **Hint**: Show best recommended move |
| `Arrows` / `WASD` | **Cursor Navigation** |
| `Enter` / `Space` | **Confirm / Pick / Drop** currently highlighted card or pile |
| `Esc` / `Backspace` / `X` | **Cancel / Clear Selection** |
| `N` | **New Game**: Deal a fresh shuffled game |
| `R` | **Restart**: Replay current deal |
| `Q` | **Quit to Arcade Hub** (auto-saves scores) |
| **Mouse Left-Click** | Click Stock to draw, click Waste/Tableau to pick or place |
| **Mouse Right-Click** | Instantly send clicked card to Foundation |

---

## Scoring Rules

- **Waste to Tableau**: +5 points
- **Waste to Foundation**: +10 points
- **Tableau to Foundation**: +10 points
- **Turn over hidden Tableau card**: +5 points
- **Foundation to Tableau**: -15 points (clamped to 0)
- **Recycle Waste into Stock**: -20 points (clamped to 0)

---

## Standalone Build & Test

```bash
# Build solitaire.exe
make

# Run automated headless unit test suite
make test

# Launch standalone game
./solitaire.exe
```

