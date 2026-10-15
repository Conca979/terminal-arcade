# Blackjack 21 CLI (Terminal Casino Edition in C)

A high-performance, authentic casino implementation of **Blackjack 21** written in C for Windows console environments with 60 FPS flicker-free TrueColor ANSI rendering.

---

## Features

- **Authentic Casino Rules**:
  - **6-Deck Shoe**: 312 cards with Fisher-Yates random shuffle and ~75% penetration cut card.
  - **Dealer Rules**: Dealer stands on all 17s (hard 17 and soft 17) and hits on 16 or lower.
  - **Natural 21 Blackjack**: Pays 3:2 immediately (`bet * 1.5`).
  - **Insurance**: Offered when dealer upcard is Ace; pays 2:1 on dealer Blackjack.
  - **Double Down**: Double bet and receive exactly 1 final card.
  - **Pair Splitting**: Split identical value cards into two independent hands with separate bets and active hand indicator.
  - **Late Surrender**: Forfeit initial 2 cards to reclaim 50% of original bet.
- **Dynamic Chip Betting Economy**:
  - Starts with $1,000 chip bankroll.
  - Quick bet presets: $10, $25, $50, $100, $500, or All-In.
  - Fine adjustment with `↑`/`↓` / `W`/`S` (±$10).
  - Automatic $500 reload grant if bankroll ever drops below minimum bet.
- **7×5 ASCII Playing Cards**:
  - Elegant rendered card boxes with corner ranks and centered suit symbols (♠ ♥ ♦ ♣).
  - Distinct suit colors: Crimson red for Hearts/Diamonds, ice cyan-white for Spades/Clubs.
  - Stylized face-down hatched pattern for the dealer's hole card.
- **Centralized Score & Bankroll Persistence**:
  - Saves current chip balance, all-time record bankroll, total hands played, and total hands won to `arcade_scores.ini`.
- **Zero Viewport Jitter**:
  - Compact 20-line rendering layout ensuring 100% flicker-free rendering on standard 80×24 and 80×25 terminals without vertical scroll overflow.

---

## Controls

### Betting Phase
| Key | Action |
| :--- | :--- |
| `↑` / `W` | Increase Bet (+$10) |
| `↓` / `S` | Decrease Bet (-$10) |
| `1` - `5` | Preset Bet ($10, $25, $50, $100, $500) |
| `A` | All-In Bet |
| `Space` / `Enter` | Confirm Bet & Deal Cards |
| `Q` / `Esc` | Auto-Save & Exit to Arcade Hub |

### Player Turn
| Key | Action |
| :--- | :--- |
| `H` / `Space` | **Hit**: Draw another card |
| `S` / `Enter` | **Stand**: Keep current total and end turn |
| `D` | **Double Down**: Double bet, draw 1 card, and stand |
| `P` / `Tab` | **Split**: Split pair into two separate hands |
| `U` | **Surrender**: Forfeit hand and reclaim 50% bet |
| `Q` / `Esc` | Auto-Save & Exit to Arcade Hub |

### Insurance Prompt
| Key | Action |
| :--- | :--- |
| `Y` / `I` | Accept Insurance (cost: 50% of bet; pays 2:1) |
| `N` / `Space` / `Enter` | Decline Insurance |

### Round Over / Game Over
| Key | Action |
| :--- | :--- |
| `Space` / `Enter` | Next Hand / Play Again |
| `R` | Reload Bankroll to $1,000 |
| `Q` / `Esc` | Auto-Save & Exit to Arcade Hub |

---

## How to Build & Run

### Using Make

```bash
cd games/blackjack
make
.\blackjack.exe
```

### Run Unit Tests

```bash
make test
```

### Direct GCC Compilation

```bash
gcc -Wall -Wextra -O2 -Iinclude -I../../include -o blackjack.exe src/main.c src/game.c src/input.c src/platform.c src/render.c -lm
.\blackjack.exe
```
