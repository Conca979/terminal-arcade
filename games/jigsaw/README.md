# 🧩 Jigsaw Snap CLI

A cozy, retro terminal jigsaw puzzle game in C featuring TrueColor pixel art, pick-and-place piece snap mechanics, dual mouse & keyboard controls, and multiple art gallery themes.

---

## Gameplay & Rules

- **4×4 Target Grid (16 Pieces)**: Reconstruct a complete 32×12 pixel artwork from 16 jigsaw chunks.
- **Piece Tray (Pool)**: Browse unplaced pieces from the tray with live mini-pixel previews.
- **Satisfying Snap Mechanics**: When a piece is placed into its correct home slot (`A1`–`D4`), it snaps with an instant checkmark and locks in place!
- **Swap Support**: Placing a piece onto an already occupied slot seamlessly swaps the pieces.
- **Deep Undo & Hints**:
  - `U`: Undo any number of placements.
  - `H`: Highlight the home destination of your held piece (or find the next unsnapped piece).
  - `P`: Toggle side-by-side full artwork preview reference.

---

## 5 Pixel Art & Challenge Themes (`T` key)

1. **🌄 Sunset Peak** (*Scenic Nature*): Snow-capped mountain peak reflecting in a serene lake under a fiery gradient twilight sky.
2. **🏰 Cyber Citadel** (*Synthwave Pixel Art*): Retro-futuristic neon fortress under a glowing synthwave moon and digital grid.
3. **🚀 Space Voyager** (*Cosmic Retro*): Space rocket blasting off into deep nebula cosmos past a distant emerald ringed planet.
4. **🐱 Cozy Tabby** (*Pixel Pet*): Cute orange tabby cat curled up sleeping peacefully on a velvet cushion.
5. **🔢 Chroma Matrix** (*Number Challenge*): 16 chromatic spectrum tiles with bold numbers (`01`–`16`) and quadrant badges.

---

## Controls

| Action | Keyboard | Mouse |
| :--- | :--- | :--- |
| **Move Cursor** | `WASD` / `Arrow Keys` / `HJKL` | Move pointer over board or tray |
| **Pick / Place Piece** | `Space` / `Enter` | **Left Click** on piece / slot |
| **Return Piece to Tray** | `X` / `Backspace` / `Delete` | **Right Click** |
| **Switch Focus** | `Tab` (Board ↔ Tray) | Click directly on Board or Tray |
| **Hint Assistant** | `H` | — |
| **Cycle Theme** | `T` / `M` | — |
| **Toggle Art Preview** | `P` | — |
| **Undo Move** | `U` / `Z` | — |
| **Restart Current Puzzle** | `R` | — |
| **Save & Quit** | `Q` / `ESC` | — |

---

## Build & Run

From repository root:
```bash
make jigsaw
.\games\jigsaw\jigsaw.exe
```

Or standalone:
```bash
cd games/jigsaw
make
.\jigsaw.exe
```

Run automated unit tests:
```bash
make test
```
