#include "render.h"
#include <stdio.h>
#include <string.h>

#define COLOR_RESET "\033[0m"
#define COLOR_GRID_BORDER "\033[38;2;75;110;160m"
#define COLOR_SIDE_BORDER "\033[38;2;80;90;120m"
#define COLOR_TITLE "\033[1;38;2;255;255;255m"
#define COLOR_LABEL "\033[1;38;2;160;180;210m"
#define COLOR_VALUE "\033[1;38;2;255;220;50m"
#define COLOR_SEL "\033[1;38;2;0;235;245m"
#define COLOR_EMPTY "\033[38;2;55;55;70m"

#define GHOST_VALID "\033[1;38;2;80;235;120m[]\033[0m"
#define GHOST_BLOCKED "\033[1;38;2;255;75;75m[]\033[0m"
#define CELL_EMPTY "\033[38;2;60;60;75m ·\033[0m"

// Vibrant block palettes matching Tetris
static const char *BLOCK_COLORS[8] = {
  COLOR_RESET,
  "\033[38;2;0;235;235m",   // 1: Cyan
  "\033[38;2;70;140;255m",  // 2: Blue
  "\033[38;2;255;155;0m",   // 3: Orange
  "\033[38;2;255;225;0m",   // 4: Yellow
  "\033[38;2;50;215;50m",   // 5: Green
  "\033[38;2;195;65;255m",  // 6: Purple
  "\033[38;2;255;65;65m"    // 7: Red
};

void render_init(void) {
  printf("\033[2J\033[H");
  fflush(stdout);
}

void render_cleanup(void) {
  printf(COLOR_RESET);
  fflush(stdout);
}

void render_frame(const GameState *game, unsigned long long current_time) {
  (void)current_time;
  static char buffer[32768];
  int offset = 0;

  offset += snprintf(buffer + offset, sizeof(buffer) - offset, "\033[H");

  // Title Banner (63 cols wide)
  offset += snprintf(
    buffer + offset, sizeof(buffer) - offset,
    "  %s╔═══════════════════════════════════════════════════════════╗%s\033[K\n"
    "  %s║%s              %sC L A S S I C   B L O C K   P U Z Z L E%s              %s║%s\033[K\n"
    "  %s╚═══════════════════════════════════════════════════════════╝%s\033[K\n",
    COLOR_GRID_BORDER, COLOR_RESET,
    COLOR_GRID_BORDER, COLOR_RESET, COLOR_TITLE, COLOR_RESET, COLOR_GRID_BORDER, COLOR_RESET,
    COLOR_GRID_BORDER, COLOR_RESET
  );

  // Status Bar
  offset += snprintf(
    buffer + offset, sizeof(buffer) - offset,
    "  %sSCORE:%s %s%06d%s   "
    "%sBEST:%s %s%06d%s   "
    "%sLINES:%s %s%03d%s   "
    "%sSTREAK:%s \033[1;32mx%d\033[0m   "
    "%sMODE:%s \033[1;38;2;255;180;50m[ENDLESS]\033[0m\033[K\n\033[K\n",
    COLOR_LABEL, COLOR_RESET, COLOR_VALUE, game->score, COLOR_RESET,
    COLOR_LABEL, COLOR_RESET, COLOR_VALUE, game->high_score, COLOR_RESET,
    COLOR_LABEL, COLOR_RESET, COLOR_VALUE, game->lines_cleared, COLOR_RESET,
    COLOR_LABEL, COLOR_RESET, game->streak,
    COLOR_LABEL, COLOR_RESET
  );

  // Prepare 12 Sidebar lines (each 34 cols inside, 36 total)
  char side[12][256];

  char streak_buf[32];
  if (game->streak > 1) {
    snprintf(streak_buf, sizeof(streak_buf), "%dx MULTIPLIER", game->streak);
  } else {
    snprintf(streak_buf, sizeof(streak_buf), "None");
  }

  char sel_str[32];
  if (game->selected_piece >= 0) {
    snprintf(sel_str, sizeof(sel_str), "SLOT [%d]", game->selected_piece + 1);
  } else {
    snprintf(sel_str, sizeof(sel_str), "None (Pick 1-3)");
  }

  char valid_str[32];
  if (game->selected_piece >= 0) {
    if (game_can_place(game, game->selected_piece, game->cursor_r, game->cursor_c)) {
      snprintf(valid_str, sizeof(valid_str), "VALID (Ready)");
    } else {
      snprintf(valid_str, sizeof(valid_str), "BLOCKED");
    }
  } else {
    snprintf(valid_str, sizeof(valid_str), "--");
  }

  snprintf(side[0], sizeof(side[0]), "%s┌── STATS & PROGRESS ──────────────┐%s", COLOR_SIDE_BORDER, COLOR_RESET);
  snprintf(side[1], sizeof(side[1]), "%s│%s  Score:         \033[1;33m%-18d\033[0m%s│%s", COLOR_SIDE_BORDER, COLOR_RESET, game->score, COLOR_SIDE_BORDER, COLOR_RESET);
  snprintf(side[2], sizeof(side[2]), "%s│%s  High Score:    \033[1;32m%-18d\033[0m%s│%s", COLOR_SIDE_BORDER, COLOR_RESET, game->high_score, COLOR_SIDE_BORDER, COLOR_RESET);
  snprintf(side[3], sizeof(side[3]), "%s│%s  Lines Cleared: %-18d%s│%s", COLOR_SIDE_BORDER, COLOR_RESET, game->lines_cleared, COLOR_SIDE_BORDER, COLOR_RESET);
  snprintf(side[4], sizeof(side[4]), "%s│%s  Streak Combo:  %-18s%s│%s", COLOR_SIDE_BORDER, COLOR_RESET, streak_buf, COLOR_SIDE_BORDER, COLOR_RESET);
  snprintf(side[5], sizeof(side[5]), "%s│%s  Blocks Placed: %-18d%s│%s", COLOR_SIDE_BORDER, COLOR_RESET, game->blocks_placed, COLOR_SIDE_BORDER, COLOR_RESET);
  snprintf(side[6], sizeof(side[6]), "%s├── SELECTED PIECE ────────────────┤%s", COLOR_SIDE_BORDER, COLOR_RESET);
  snprintf(side[7], sizeof(side[7]), "%s│%s  Active Slot:   \033[1;36m%-18s\033[0m%s│%s", COLOR_SIDE_BORDER, COLOR_RESET, sel_str, COLOR_SIDE_BORDER, COLOR_RESET);
  snprintf(side[8], sizeof(side[8]), "%s│%s  Placement:     %-18s%s│%s", COLOR_SIDE_BORDER, COLOR_RESET, valid_str, COLOR_SIDE_BORDER, COLOR_RESET);
  snprintf(side[9], sizeof(side[9]), "%s├── QUICK SHORTCUTS ───────────────┤%s", COLOR_SIDE_BORDER, COLOR_RESET);
  snprintf(side[10], sizeof(side[10]), "%s│%s  1-3: Pick Piece  Space: Place  %s│%s", COLOR_SIDE_BORDER, COLOR_RESET, COLOR_SIDE_BORDER, COLOR_RESET);
  snprintf(side[11], sizeof(side[11]), "%s└──────────────────────────────────┘%s", COLOR_SIDE_BORDER, COLOR_RESET);

  // Compute ghost placement overlay
  bool is_ghost[GRID_SIZE][GRID_SIZE] = {false};
  bool ghost_valid = false;

  if (game->selected_piece >= 0 && game->tray[game->selected_piece].available) {
    const Shape *s = &game->tray[game->selected_piece].shape;
    ghost_valid = game_can_place(game, game->selected_piece, game->cursor_r, game->cursor_c);
    for (int dr = 0; dr < s->h; dr++) {
      for (int dc = 0; dc < s->w; dc++) {
        if (s->mask[dr][dc] == 1) {
          int tr = game->cursor_r + dr;
          int tc = game->cursor_c + dc;
          if (tr >= 0 && tr < GRID_SIZE && tc >= 0 && tc < GRID_SIZE) {
            is_ghost[tr][tc] = true;
          }
        }
      }
    }
  }

  // Render Grid Line 0 (Top border: 20 chars wide) side-by-side with Sidebar Line 0
  int side_idx = 0;
  offset += snprintf(
    buffer + offset, sizeof(buffer) - offset,
    "  %s╔════════════════════╗%s   %s\033[K\n",
    COLOR_GRID_BORDER, COLOR_RESET, side[side_idx++]
  );

  // Render 10 Rows of the Grid
  for (int r = 0; r < GRID_SIZE; r++) {
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "  %s║%s", COLOR_GRID_BORDER, COLOR_RESET);

    for (int c = 0; c < GRID_SIZE; c++) {
      if (is_ghost[r][c]) {
        if (ghost_valid) {
          offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s", GHOST_VALID);
        } else {
          offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s", GHOST_BLOCKED);
        }
      } else if (game->grid[r][c] > 0) {
        int color_idx = game->grid[r][c];
        if (color_idx < 1 || color_idx > 7) color_idx = 1;
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s██%s", BLOCK_COLORS[color_idx], COLOR_RESET);
      } else {
        // Empty cell
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s", CELL_EMPTY);
      }
    }

    const char *s_line = (side_idx < 12) ? side[side_idx++] : "";
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s║%s   %s\033[K\n", COLOR_GRID_BORDER, COLOR_RESET, s_line);
  }

  // Render Grid Line 11 (Bottom border) side-by-side with Sidebar Line 11
  const char *s_bottom = (side_idx < 12) ? side[side_idx++] : "";
  offset += snprintf(
    buffer + offset, sizeof(buffer) - offset,
    "  %s╚════════════════════╝%s   %s\033[K\n",
    COLOR_GRID_BORDER, COLOR_RESET, s_bottom
  );

  // Combo announcement or prompt
  if (game->combo_timer > 0 && game->combo_banner[0] != '\0') {
    offset += snprintf(
      buffer + offset, sizeof(buffer) - offset,
      "  \033[1;38;2;255;220;50;48;2;50;35;20m ★★★ %s ★★★ \033[0m\033[K\n",
      game->combo_banner
    );
  } else {
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "  %sCHOOSE A PIECE BELOW TO PLACE ON THE BOARD:%s\033[K\n", COLOR_LABEL, COLOR_RESET);
  }

  // Render the 3 Candidate Piece Trays below the grid
  // Outer width of each box: 16 chars. Gaps: 3 chars.
  // Box 0 (cols 2..17), Box 1 (cols 21..36), Box 2 (cols 40..55)
  char tray_top[3][64];
  char tray_bot[3][64];
  const char *tray_border_col[3];

  for (int i = 0; i < MAX_PIECES_TRAY; i++) {
    if (game->selected_piece == i) {
      tray_border_col[i] = COLOR_SEL;
      snprintf(tray_top[i], sizeof(tray_top[i]), "╔═ [%d] PIECE ═╗", i + 1);
      snprintf(tray_bot[i], sizeof(tray_bot[i]), "╚══════════════╝");
    } else {
      tray_border_col[i] = COLOR_SIDE_BORDER;
      snprintf(tray_top[i], sizeof(tray_top[i]), "┌─ [%d] PIECE ─┐", i + 1);
      snprintf(tray_bot[i], sizeof(tray_bot[i]), "└──────────────┘");
    }
  }

  // Tray Top Borders
  offset += snprintf(
    buffer + offset, sizeof(buffer) - offset,
    "  %s%s%s   %s%s%s   %s%s%s\033[K\n",
    tray_border_col[0], tray_top[0], COLOR_RESET,
    tray_border_col[1], tray_top[1], COLOR_RESET,
    tray_border_col[2], tray_top[2], COLOR_RESET
  );

  // Tray Content Rows (5 rows for up to 5x5 polyomino shapes, interior width = 14)
  for (int row = 0; row < MAX_SHAPE_DIM; row++) {
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "  ");

    for (int t = 0; t < MAX_PIECES_TRAY; t++) {
      const TrayPiece *tp = &game->tray[t];
      const char *b_col = tray_border_col[t];

      offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s│%s", b_col, COLOR_RESET);

      if (!tp->available) {
        if (row == 2) {
          offset += snprintf(buffer + offset, sizeof(buffer) - offset, "\033[38;2;120;110;100m   [PLACED]   \033[0m");
        } else {
          offset += snprintf(buffer + offset, sizeof(buffer) - offset, "              ");
        }
      } else {
        const Shape *s = &tp->shape;
        int color_idx = s->color_id;
        if (color_idx < 1 || color_idx > 7) color_idx = 1;

        // Interior width = 14 characters. Each cell is 2 characters wide.
        int shape_w_chars = s->w * 2;
        int pad_left = (14 - shape_w_chars) / 2;
        int pad_right = 14 - pad_left - shape_w_chars;
        int pad_top = (5 - s->h) / 2;

        int s_row = row - pad_top;

        if (s_row >= 0 && s_row < s->h) {
          for (int c = 0; c < pad_left; c++) {
            offset += snprintf(buffer + offset, sizeof(buffer) - offset, " ");
          }
          for (int c = 0; c < s->w; c++) {
            if (s->mask[s_row][c] == 1) {
              offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s██%s", BLOCK_COLORS[color_idx], COLOR_RESET);
            } else {
              offset += snprintf(buffer + offset, sizeof(buffer) - offset, "  ");
            }
          }
          for (int c = 0; c < pad_right; c++) {
            offset += snprintf(buffer + offset, sizeof(buffer) - offset, " ");
          }
        } else {
          offset += snprintf(buffer + offset, sizeof(buffer) - offset, "              ");
        }
      }

      offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s│%s", b_col, COLOR_RESET);
      if (t < MAX_PIECES_TRAY - 1) {
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "   ");
      }
    }

    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "\033[K\n");
  }

  // Tray Bottom Borders
  offset += snprintf(
    buffer + offset, sizeof(buffer) - offset,
    "  %s%s%s   %s%s%s   %s%s%s\033[K\n\033[K\n",
    tray_border_col[0], tray_bot[0], COLOR_RESET,
    tray_border_col[1], tray_bot[1], COLOR_RESET,
    tray_border_col[2], tray_bot[2], COLOR_RESET
  );

  // Game Over Banner
  if (game->game_over) {
    offset += snprintf(
      buffer + offset, sizeof(buffer) - offset,
      "  \033[1;38;2;255;70;70;48;2;50;20;20m"
      " ★★★ GAME OVER! NO MORE VALID MOVES! Final Score: %d [R: New Game | Q: Quit] ★★★ "
      "\033[0m\033[K\n",
      game->score
    );
  } else {
    // Controls Legend
    offset += snprintf(
      buffer + offset, sizeof(buffer) - offset,
      "  %sControls:%s\033[K\n"
      "    %s[Mouse]%s Click piece to pick, hover to preview, click grid to place block\033[K\n"
      "    %s[Keyboard]%s [1-3] Select piece   [Arrows/WASD] Move   [Space] Place   [R] Reset   [Q] Quit\033[K\n",
      COLOR_LABEL, COLOR_RESET,
      COLOR_TITLE, COLOR_LABEL,
      COLOR_TITLE, COLOR_LABEL
    );
  }

  offset += snprintf(buffer + offset, sizeof(buffer) - offset, "\033[J");

  fwrite(buffer, 1, offset, stdout);
  fflush(stdout);
}
