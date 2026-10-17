# Terminal Arcade in C — Developer & Agent Architecture Guide

A modular, high-performance collection of classic arcade and puzzle games written in C for Windows console environments with 60 FPS flicker-free ANSI color rendering.

---

## Standard 5-Module Game Architecture

Every game in `games/<game_name>/` follows a strict modular 5-component layout:

```
games/<game_name>/
├── include/
│   ├── config.h        # Constants, board sizes, enums, game states, action codes
│   ├── game.h          # Data structures, GameState model, pure logic prototypes
│   ├── input.h         # Input action mapping interface
│   ├── platform.h      # Console mode, QPC high-res timer, sleep interface
│   └── render.h        # ANSI TrueColor frame rendering interface
├── src/
│   ├── config.c        # (Optional) Static asset tables, dictionaries, word lists
│   ├── game.c          # Pure game logic, physics, board state, rules, score calculation
│   ├── input.c         # Non-blocking key decoding (_kbhit / _getch) -> GameAction
│   ├── platform.c      # Windows console VT100, UTF-8, QPC timing, cleanup hooks
│   ├── render.c        # Flicker-free TrueColor ANSI rendering, compact layout (<= 21 lines)
│   └── main.c          # 60 FPS game loop (Init -> Input -> Update -> Render -> Save)
├── test_<game_name>.c  # Headless automated unit tests (pure logic / no console GUI)
├── Makefile            # Standalone build (all, test, clean targets)
└── README.md           # User guide, gameplay rules, and player controls
```

### Module Responsibilities

1. **`config.h`**:
   - Defines all game constants (board dimensions, physics constants, presets, tick rates).
   - Declares state enums (`RoundState`, `PlayState`) and input semantic codes (`GameAction`).
2. **`game.h` / `game.c`**:
   - Encapsulates pure state (`GameState`) and deterministic simulation logic.
   - Independent of rendering and direct console I/O; 100% testable in headless suites.
   - Handles game reset, physics integration, collisions, rule evaluation, score calculation, and savegame serialization.
3. **`input.h` / `input.c`**:
   - Non-blocking keyboard decoding using `_kbhit()` and `_getch()`.
   - Mouse support (e.g. aiming/clicking in Breaker or Block) decoded via Win32 `ReadConsoleInputW` / `MOUSE_EVENT`.
   - Handles extended key codes (arrows: `0` or `224` prefix).
   - Normalizes raw events into semantic `GameAction` enums.
4. **`platform.h` / `platform.c`**:
   - Windows Console API terminal configuration and lifecycle management.
   - Enables virtual terminal sequences (`ENABLE_VIRTUAL_TERMINAL_PROCESSING`).
   - High-resolution timing via `QueryPerformanceCounter` (QPC).
   - Cleanup guarantees on process exit or signal interrupts.
5. **`render.h` / `render.c`**:
   - TrueColor ANSI rendering (`\033[38;2;R;G;Bm` fg, `\033[48;2;R;G;Bm` bg).
   - Compact viewport budgets (strictly `<= 21 lines`) to prevent vertical scrolling/jitter on standard 80×24 and 80×25 terminals.
   - Dynamic visible length calculation (`visible_length()`) to ignore ANSI escapes and UTF-8 continuation bytes for precise box alignment.
   - Appends `\033[K` before newlines to clear trailing characters from prior frames.
6. **`main.c`**:
   - Coordinates game lifecycle: `platform_init()` -> `render_init()` -> `game_init()` -> 60 FPS Loop -> `game_save_scores()` -> `render_cleanup()` -> `platform_cleanup()`.
7. **`test_<game>.c`**:
   - Automated headless regression tests using standard `<assert.h>`.

---

## Technical Constraints & Implementation Guidelines

### 1. Language Standards & Formatting
- **Standard**: C99 / C11 pure standard library + Windows Console API.
- **Indentation**: Strict **2 spaces** indentation throughout the entire codebase (no tabs).
- **Naming Conventions**: `snake_case` for functions/variables, `UPPER_SNAKE_CASE` for constants/macros, `PascalCase` for typedef structs/enums.

### 2. Windows Console & VT100 ANSI Setup
All games must initialize and restore console state cleanly:
- **Output Mode**: Enable `ENABLE_VIRTUAL_TERMINAL_PROCESSING | ENABLE_PROCESSED_OUTPUT`.
- **Input Mode**: Disable `ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT` for immediate non-blocking raw keypresses (enable `ENABLE_MOUSE_INPUT | ENABLE_EXTENDED_FLAGS` when mouse support is required).
- **Code Page**: Set UTF-8 output code page: `SetConsoleOutputCP(CP_UTF8)`.
- **Alternate Buffer & Cursor**: Enter alternate buffer `\033[?1049h` and hide cursor `\033[?25l` on init; restore cursor `\033[?25h` and exit buffer `\033[?1049l` on cleanup.

### 3. Cleanup Guarantees
Console state MUST be restored on exit to prevent bricking the user's terminal:
- Register `atexit(platform_cleanup)`.
- Register console control handler via `SetConsoleCtrlHandler(console_ctrl_handler, TRUE)` to catch `Ctrl+C`, window close, and break signals.
- Always reset terminal attributes `\033[0m` and flush `stdout`.

### 4. Centralized Score Persistence (`include/scores.h`) & Save States
All arcade games share a unified score repository stored in [arcade_scores.ini](file:///c:/Users/admin/projects/personal/tetris/arcade_scores.ini):
- **Isolation**: Each game owns a dedicated INI section (e.g. `[tetris]`, `[minesweeper]`, `[wordle]`, `[blackjack]`).
- **Path Resolution**: `arcade_scores_path()` automatically resolves the repository root across execution contexts (Arcade Hub launcher, standalone game executable in `games/<name>/`, or test runner).
- **API**:
  - `int arcade_score_get(const char *section, const char *key)`: Returns score value or 0.
  - `void arcade_score_set(const char *section, const char *key, int value)`: Reads, updates, and atomically saves INI.
- Scores must be saved on round completion and game exit.
- **In-Progress Save States**: Games supporting session resume (e.g. 2048, Sudoku, Wood Block) must store serialized save files locally within `games/<game_name>/` (e.g. `games/<game_name>/save_<game_name>.dat`) to keep the repository root clean.

### 5. Viewport & Rendering Constraints
- **Vertical Line Budget**: Total rendered lines must be strictly `<= 21 lines` to guarantee zero scrolling, screen jitter, or rolling on standard 80×24 / 80×25 terminals.
- **ANSI Length Calculation**: Never use `strlen()` to pad formatted lines containing ANSI escape sequences or UTF-8 glyphs. Implement `visible_length()` to skip `\033...m` escape codes and UTF-8 continuation bytes (`(c & 0xC0) == 0x80`).

---

## Adding a New Game to the Arcade (Checklist)

1. **Create Game Subdirectory**:
   `games/<game_name>/` containing `include/`, `src/`, `test_<game_name>.c`, `Makefile`, and `README.md`.
2. **Implement Standalone Makefile**:
   Must provide `all` (`<game_name>.exe`), `test` (`test_<game_name>.exe`), and `clean` targets. Include `-Iinclude -I../../include`.
3. **Register in Master [Makefile](file:///c:/Users/admin/projects/personal/tetris/Makefile)**:
   Add game target to `all`, `test`, `clean`, and `.PHONY`.
4. **Register in Arcade Hub ([src/main.c](file:///c:/Users/admin/projects/personal/tetris/src/main.c))**:
   Append an entry to `GAMES[]` table with title, description, binary path, score section, and score key.
5. **Score Persistence**:
   Read and update high scores via `arcade_score_get()` and `arcade_score_set()` from [include/scores.h](file:///c:/Users/admin/projects/personal/tetris/include/scores.h).
6. **Documentation**:
   Add user-facing game description, controls, and features to root [README.md](file:///c:/Users/admin/projects/personal/tetris/README.md) and `games/<game_name>/README.md`.

---

## Build Pipeline Reference

- **Build Arcade Hub & All Games**:
  ```bash
  make
  ```
  Produces `arcade.exe` launcher and all game executables in `games/<game>/`.

- **Run Full Automated Test Suite**:
  ```bash
  make test
  ```
  Executes unit tests across all games in sequence.

- **Build Individual Games**:
  ```bash
  make tetris
  make 2048
  make minesweeper
  make snake
  make invaders
  make sudoku
  make breaker
  make block
  make pong
  make typing
  make wordle
  make flappy
  make blackjack
  make solitaire
  ```

- **Run Individual Game Tests**:
  ```bash
  make -C games/<game_name> test
  ```

- **Clean Entire Workspace**:
  ```bash
  make clean
  ```
