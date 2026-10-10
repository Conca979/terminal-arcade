# Tetris Terminal (C Edition)

A modern, guideline-compliant terminal Tetris game written in C for Windows console environments with 60 FPS flicker-free ANSI color rendering.

![C Standard](https://img.shields.io/badge/Language-C99%2FC11-blue.svg)
![Platform](https://img.shields.io/badge/Platform-Windows%2010%2F11-0078D6.svg)
![Build](https://img.shields.io/badge/Build-GCC%20%2F%20Make-brightgreen.svg)

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
- **Scoring & Progression**: Traditional scoring multipliers (Single, Double, Triple, Tetris) with speed progression across levels.
- **High Score Persistence**: Automatically saved to and loaded from `tetris_highscore.txt`.
- **Flicker-Free ANSI Rendering**: Double-buffered frame output with no screen tearing.
- **Dual Controls**: Supports both standard Arrow keys and ergonomic WASD layouts.

---

## Prerequisites

To build and run Tetris Terminal, you will need:

1. **Operating System**: Windows 10 or Windows 11 (Windows Terminal, PowerShell, or Command Prompt).
2. **C Compiler**: GCC (via [MSYS2](https://www.msys2.org/), MinGW-w64, or Chocolatey).
3. **Make** (Optional, but recommended): `make` or `mingw32-make`.

> [!TIP]
> If you have **MSYS2** installed:
> ```bash
> pacman -S mingw-w64-ucrt-x86_64-gcc make
> ```
> Ensure `C:\msys64\ucrt64\bin` is added to your system `PATH`.

---

## Setup & Installation

### 1. Clone the Repository

```bash
git clone https://github.com/Conca979/tetris-terminal.git
cd tetris-terminal
```

### 2. Build the Game

Using **Make**:
```bash
make
```

Or compiling directly with **GCC**:
```bash
gcc -Wall -Wextra -O2 -Iinclude -o tetris.exe src/main.c src/game.c src/input.c src/platform.c src/render.c
```

### 3. Run the Game

```powershell
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

---

## Running Tests

An automated test suite is included to validate collision logic, SRS wall kicks, drops, line clearing, and the hold system:

```bash
make test
```

---

## Project Structure

```
tetris-terminal/
├── include/
│   ├── config.h       # Game constants, types, states, and controls
│   ├── game.h         # Game logic, state definitions, and SRS declarations
│   ├── input.h        # Non-blocking console input interface
│   ├── platform.h     # Windows console and timer abstractions
│   └── render.h       # Double-buffered ANSI renderer interface
├── src/
│   ├── game.c         # SRS kicks, 7-bag generator, lock delay, collision
│   ├── input.c        # Keyboard event handler
│   ├── main.c         # 60 FPS pacing game loop
│   ├── platform.c     # Win32 console modes, UTF-8, and signal handlers
│   └── render.c       # Playfield, ghost piece, overlays, and color formatting
├── test_tetris.c      # Unit test runner
├── Makefile           # Build and test pipeline
├── AGENTS.md          # Architecture and module documentation
└── README.md          # Project overview and setup instructions
```

---

## License

This project is open source and available under the [MIT License](LICENSE).
