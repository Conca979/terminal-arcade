# Wordle CLI

A faithful, high-performance terminal implementation of the classic 5-letter word puzzle game written in C for Windows console environments with 60 FPS ANSI TrueColor rendering.

## Game Rules & Features

- **6 Attempts to Guess the Secret 5-Letter Word**:
  - Hidden solution word selected from a curated list of 476 English words (`solution_words.c`).
  - Guesses validated against a comprehensive dictionary of 14,800+ 5-letter English words (`valid-wordle-words.txt`).
- **Standard Wordle Color Clues**:
  - **🟩 Green (`CORRECT`)**: Letter is in the word and in the exact spot.
  - **🟨 Yellow (`PRESENT`)**: Letter is in the word but in a different spot.
  - **⬛ Gray (`ABSENT`)**: Letter does not occur in the remaining positions.
- **Strict Duplicate Letter Algorithm**:
  - Two-pass frequency counting matching official Wordle rules (exact matches resolve first, followed by remaining letter frequency allocations).
- **Side-by-Side Live Statistics Panel**:
  - Games played, Win rate %, Current streak, and Best streak.
  - Live 6-bar guess distribution chart.
- **QWERTY Virtual Keyboard Clue Tracker**:
  - On-screen keyboard automatically updates key colors (Green > Yellow > Gray > Default).
- **Persistent State & Auto-Save**:
  - Quitting midway through a puzzle saves in-progress guesses to `savegame.dat` and automatically resumes when re-entered.
  - High scores and streak stats recorded centrally in `arcade_scores.ini` under `[wordle]`.

## Controls

| Key | Action |
| :--- | :--- |
| `A` – `Z` | Type letters into active guess row |
| `Backspace` / `Del` | Erase last letter |
| `Enter` | Submit guess / Play next round |
| `Space` | Play next round on Game Over |
| `R` / `Ctrl+R` | Start fresh puzzle |
| `Esc` / `Ctrl+Q` | Auto-save active game & return to Arcade Hub |

## Standalone Build & Test

```powershell
# Build wordle.exe
make

# Run automated test suite
make test

# Launch standalone game
.\wordle.exe
```
