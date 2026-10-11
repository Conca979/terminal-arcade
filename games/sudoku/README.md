# Sudoku CLI

A terminal-based Sudoku puzzle game featuring intelligent board generation, 4 difficulty levels, candidate pencil notes mode, an active cell inspector panel, conflict detection, and 64-step undo history.

---

## Features

- **Procedural Puzzle Generator**: Generates uniquely solvable puzzles dynamically with four difficulty presets:
  - **Easy**: ~39 clues
  - **Medium**: ~31 clues
  - **Hard**: ~26 clues
  - **Expert**: ~23 clues
- **Candidate Pencil Notes**: Toggle note mode (`N`) to write small candidate notes (`1`–`9`) into any cell without committing a permanent value.
- **Side-by-Side Cell Inspector Panel**:
  - Displays currently selected row/col and value status.
  - Lists valid candidates for the hovered cell.
  - Shows real-time counts of remaining numbers (`1` to `9`) needed across the board with `DONE` completion badges.
- **Smart Board Highlighting**:
  - **Crosshair Highlighting**: Dimmed background for the active cell's row, column, and 3x3 block.
  - **Digit Matching**: Hovering over any filled digit automatically highlights all identical digits on the board.
  - **Error & Conflict Detection**: Live red indicator highlights duplicate digits sharing the same row, column, or block.
- **64-Step Undo Stack**: Press `U` or `Z` at any time to rewind moves, value entries, or pencil note edits.
- **Hint System**: Press `H` to reveal the solution for any stuck cell.
- **Auto-Save & Seamless Resuming**: Active puzzles, timer, pencil notes, difficulty, and undo history are auto-saved to `savegame.dat`. Resumes instantly upon launching the game.

---

## Controls

| Key | Action |
| --- | --- |
| `↑` / `↓` / `←` / `→` or `WASD` / `HJKL` | Move cursor across 9x9 grid |
| `1` – `9` | Place digit / Toggle note (in Note mode) |
| `N` | Toggle Pencil Notes mode |
| `0` / `Backspace` / `Del` / `X` | Clear value / notes in cell |
| `U` / `Z` | Undo move (up to 64 steps) |
| `H` | Hint (reveals correct cell value) |
| `E` | Toggle real-time conflict checking |
| `Tab` / `M` | Cycle difficulty (Easy / Medium / Hard / Expert) |
| `P` | Pause / resume game |
| `R` | Start fresh puzzle (clears active save) |
| `Q` / `Esc` | Auto-save and exit to Arcade Hub |

---

## Build & Run

```bash
# Build standalone Sudoku executable
make

# Run Sudoku CLI
./sudoku.exe

# Run unit test suite
make test
```
