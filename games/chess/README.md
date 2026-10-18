# Chess Master CLI (with AI Bot)

A terminal Chess game written in C featuring complete FIDE rules, TrueColor Unicode board rendering, mouse and keyboard support, Standard Algebraic Notation (SAN) move history, and a built-in Negamax AI engine with Alpha-Beta pruning and Quiescence search.

## Features

- **Full Chess Rules**:
  - Legal move validation (pinned pieces, check evasion, discovered check prevention).
  - **Castling** (Kingside `O-O` and Queenside `O-O-O`) with attack-path validation.
  - **En Passant** capture tracking and single-turn expiration.
  - **Pawn Promotion** with interactive piece selection (`Queen`, `Rook`, `Bishop`, `Knight`).
  - **Endgame Detection**: Checkmate, Stalemate, 50-move rule, and Insufficient Material.
- **Built-in AI Engine**:
  - **Negamax + Alpha-Beta Pruning**: Fast minimax tree search with MVV-LVA move ordering.
  - **Quiescence Search**: Extends search across capture sequences at leaf nodes to prevent tactical horizon blunders.
  - **Positional Evaluation**: Material balance + Michniewski Piece-Square Tables (PST) with middlegame/endgame King transition and bishop-pair bonus.
  - **4 Play Modes**: `AI Easy`, `AI Medium`, `AI Hard`, and `2-Player` local pass-and-play.
- **Interactive UI**:
  - Click-to-select and click-to-move mouse support + keyboard cursor navigation.
  - Highlighted legal target squares (`•` for quiet moves, `[x]` for captures) and last-move highlights.
  - Live dashboard with captured piece trays, material advantage, and SAN move log.

## Controls

| Input | Action |
| :--- | :--- |
| **Mouse Left Click** | Select piece / Move to highlighted square |
| **Mouse Right Click** | Cancel current piece selection |
| **Arrow Keys / WASD** | Move board cursor |
| **Space / Enter** | Select piece at cursor / Confirm move |
| **Backspace / X / C** | Cancel current selection |
| **M** | Cycle Mode (`AI Easy` -> `AI Medium` -> `AI Hard` -> `2-Player`) |
| **F** | Flip board orientation (White / Black perspective) |
| **U / Z** | Undo move (undoes full AI + Human turn pair in AI mode) |
| **H** | Request AI move hint |
| **N / R** | Start a new game |
| **1 / 2 / 3 / 4** | Promote pawn to Queen / Rook / Bishop / Knight |
| **Q / ESC** | Save stats and exit |

## Build & Run

```bash
make
./chess.exe
```

## Run Unit Tests

```bash
make test
```
