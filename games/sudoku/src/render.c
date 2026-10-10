#include "render.h"
#include <stdio.h>
#include <string.h>

#define COLOR_RESET "\033[0m"
#define COLOR_BOX_BORDER "\033[38;2;85;125;195m"
#define COLOR_CELL_BORDER "\033[38;2;55;75;105m"
#define COLOR_TITLE "\033[1;38;2;255;255;255m"
#define COLOR_LABEL "\033[1;38;2;160;185;220m"
#define COLOR_VALUE "\033[1;38;2;255;220;50m"
#define COLOR_ACCENT "\033[1;38;2;0;225;225m"
#define COLOR_DIM "\033[38;2;120;135;160m"
#define COLOR_DONE "\033[1;38;2;80;220;100m"

// Cell formatting
#define COLOR_GIVEN "\033[1;38;2;245;245;255m"
#define COLOR_USER "\033[1;38;2;0;230;240m"
#define COLOR_NOTE "\033[38;2;245;185;60m"

// Highlights
#define BG_CURSOR "\033[48;2;45;80;140;1;38;2;255;255;255m"
#define BG_CROSSHAIR "\033[48;2;25;35;48m"
#define BG_MATCH "\033[48;2;65;55;20;1;38;2;255;225;60m"
#define BG_ERROR "\033[48;2;135;25;25;1;38;2;255;160;160m"

static const char *DIFF_NAMES[DIFF_COUNT] = {
  "EASY", "MEDIUM", "HARD", "EXPERT"
};

void render_init(void) {
  printf("\033[2J\033[H");
  fflush(stdout);
}

void render_cleanup(void) {
  printf(COLOR_RESET);
  fflush(stdout);
}

static void format_cell(
  const GameState *game,
  int r,
  int c,
  uint8_t sel_val,
  char *out,
  size_t out_sz
) {
  const Cell *cell = &game->grid[r][c];
  bool is_cursor = (r == game->cursor_r && c == game->cursor_c);
  bool is_same_row = (r == game->cursor_r);
  bool is_same_col = (c == game->cursor_c);
  bool is_same_box = ((r / BOX_SIZE == game->cursor_r / BOX_SIZE) &&
                      (c / BOX_SIZE == game->cursor_c / BOX_SIZE));
  bool is_crosshair = (is_same_row || is_same_col || is_same_box) && !is_cursor;
  bool is_match = (sel_val > 0 && cell->value == sel_val && !is_cursor);
  bool is_error = cell->is_error && game->show_errors;

  const char *bg = "";
  if (is_cursor) {
    bg = BG_CURSOR;
  } else if (is_error) {
    bg = BG_ERROR;
  } else if (is_match) {
    bg = BG_MATCH;
  } else if (is_crosshair) {
    bg = BG_CROSSHAIR;
  }

  if (cell->value > 0) {
    const char *fg = cell->is_given ? COLOR_GIVEN : COLOR_USER;
    if (is_cursor || is_error || is_match) {
      snprintf(out, out_sz, "%s %d %s", bg, cell->value, COLOR_RESET);
    } else if (is_crosshair) {
      snprintf(out, out_sz, "%s%s %d %s", bg, fg, cell->value, COLOR_RESET);
    } else {
      snprintf(out, out_sz, "%s %d %s", fg, cell->value, COLOR_RESET);
    }
  } else {
    // Empty cell
    if (cell->notes != 0) {
      // Show note indicator (e.g. ° or single note or dot)
      int note_count = 0;
      int first_note = 0;
      for (int n = 1; n <= 9; n++) {
        if (cell->notes & (1 << n)) {
          note_count++;
          if (first_note == 0) first_note = n;
        }
      }
      if (note_count == 1) {
        snprintf(out, out_sz, "%s%s'%d'%s", bg, COLOR_NOTE, first_note, COLOR_RESET);
      } else {
        snprintf(out, out_sz, "%s%s · %s", bg, COLOR_NOTE, COLOR_RESET);
      }
    } else {
      if (bg[0] != '\0') {
        snprintf(out, out_sz, "%s   %s", bg, COLOR_RESET);
      } else {
        snprintf(out, out_sz, "   ");
      }
    }
  }
}

void render_frame(const GameState *game, unsigned long long current_time) {
  static char buffer[32768];
  int offset = 0;

  offset += snprintf(buffer + offset, sizeof(buffer) - offset, "\033[H");

  // Title Banner
  offset += snprintf(
    buffer + offset, sizeof(buffer) - offset,
    "  %s╔═══════════════════════════════════════════════════════════════════════════╗%s\n"
    "  %s║%s                      %sS U D O K U   A R C A D E%s                        %s║%s\n"
    "  %s╚═══════════════════════════════════════════════════════════════════════════╝%s\n",
    COLOR_BOX_BORDER, COLOR_RESET,
    COLOR_BOX_BORDER, COLOR_RESET, COLOR_TITLE, COLOR_RESET, COLOR_BOX_BORDER, COLOR_RESET,
    COLOR_BOX_BORDER, COLOR_RESET
  );

  // Status Bar
  int elapsed = game_get_elapsed_seconds(game, current_time);
  int mins = elapsed / 60;
  int secs = elapsed % 60;

  const char *mode_str = game->note_mode ? "\033[1;33m[PENCIL NOTES]\033[0m" : "\033[1;36m[NORMAL ENTRY]\033[0m";
  const char *err_str = game->show_errors ? "\033[1;32m[ON]\033[0m" : "\033[38;2;120;120;120m[OFF]\033[0m";

  offset += snprintf(
    buffer + offset, sizeof(buffer) - offset,
    "  %sDIFF:%s \033[1;38;2;0;225;225m[%s]\033[0m   "
    "%sTIME:%s %s%02d:%02d%s   "
    "%sMOVES:%s %s%03d%s   "
    "%sMISTAKES:%s \033[1;31m%d\033[0m   "
    "%sCHECK:%s %s   "
    "%sMODE:%s %s\n\n",
    COLOR_LABEL, COLOR_RESET, DIFF_NAMES[game->difficulty],
    COLOR_LABEL, COLOR_RESET, COLOR_VALUE, mins, secs, COLOR_RESET,
    COLOR_LABEL, COLOR_RESET, COLOR_VALUE, game->moves_made, COLOR_RESET,
    COLOR_LABEL, COLOR_RESET, game->mistakes,
    COLOR_LABEL, COLOR_RESET, err_str,
    COLOR_LABEL, COLOR_RESET, mode_str
  );

  uint8_t sel_val = game->grid[game->cursor_r][game->cursor_c].value;
  uint16_t cursor_notes = game->grid[game->cursor_r][game->cursor_c].notes;
  uint16_t candidates = game_get_valid_candidates(game, game->cursor_r, game->cursor_c);
  bool is_cur_given = game->grid[game->cursor_r][game->cursor_c].is_given;

  // Prepare sidebar lines
  char side[25][256];
  for (int i = 0; i < 25; i++) {
    side[i][0] = '\0';
  }

  snprintf(side[0], sizeof(side[0]), "%s┌── CELL INSPECTOR ─────────────────┐%s", COLOR_BOX_BORDER, COLOR_RESET);
  snprintf(side[1], sizeof(side[1]), "%s│%s Pos:  Row %d, Col %d                %s│%s",
           COLOR_BOX_BORDER, COLOR_RESET, game->cursor_r + 1, game->cursor_c + 1, COLOR_BOX_BORDER, COLOR_RESET);

  if (sel_val > 0) {
    snprintf(side[2], sizeof(side[2]), "%s│%s Val:  %s%d%s %-20s  %s│%s",
             COLOR_BOX_BORDER, COLOR_RESET,
             is_cur_given ? COLOR_GIVEN : COLOR_USER,
             sel_val, COLOR_RESET,
             is_cur_given ? "[GIVEN CLUE]" : "[USER VALUE]",
             COLOR_BOX_BORDER, COLOR_RESET);
  } else {
    snprintf(side[2], sizeof(side[2]), "%s│%s Val:  \033[38;2;120;130;140m(Empty)\033[0m                   %s│%s",
             COLOR_BOX_BORDER, COLOR_RESET, COLOR_BOX_BORDER, COLOR_RESET);
  }

  // Notes line
  char notes_buf[32] = "";
  int nb_len = 0;
  for (int n = 1; n <= 9; n++) {
    if (cursor_notes & (1 << n)) {
      nb_len += snprintf(notes_buf + nb_len, sizeof(notes_buf) - nb_len, "%d ", n);
    }
  }
  if (nb_len == 0) snprintf(notes_buf, sizeof(notes_buf), "None");
  snprintf(side[3], sizeof(side[3]), "%s│%s Notes: %-23s  %s│%s",
           COLOR_BOX_BORDER, COLOR_RESET, notes_buf, COLOR_BOX_BORDER, COLOR_RESET);

  // Candidates line
  char cand_buf[48] = "";
  int cb_len = 0;
  if (sel_val == 0) {
    for (int n = 1; n <= 9; n++) {
      if (candidates & (1 << n)) {
        cb_len += snprintf(cand_buf + cb_len, sizeof(cand_buf) - cb_len, "%d", n);
      } else {
        cb_len += snprintf(cand_buf + cb_len, sizeof(cand_buf) - cb_len, "·");
      }
    }
  } else {
    snprintf(cand_buf, sizeof(cand_buf), "Cell filled");
  }
  snprintf(side[4], sizeof(side[4]), "%s│%s Valid: %-23s  %s│%s",
           COLOR_BOX_BORDER, COLOR_RESET, cand_buf, COLOR_BOX_BORDER, COLOR_RESET);

  snprintf(side[5], sizeof(side[5]), "%s├── REMAINING DIGITS ───────────────┤%s", COLOR_BOX_BORDER, COLOR_RESET);

  for (int pair = 0; pair < 5; pair++) {
    int d1 = pair * 2 + 1;
    int d2 = pair * 2 + 2;

    char p1[32], p2[32];
    if (game->remaining_counts[d1] == 0) {
      snprintf(p1, sizeof(p1), "%d: %sDONE%s", d1, COLOR_DONE, COLOR_RESET);
    } else {
      snprintf(p1, sizeof(p1), "%d: %d left", d1, game->remaining_counts[d1]);
    }

    if (d2 <= 9) {
      if (game->remaining_counts[d2] == 0) {
        snprintf(p2, sizeof(p2), "%d: %sDONE%s", d2, COLOR_DONE, COLOR_RESET);
      } else {
        snprintf(p2, sizeof(p2), "%d: %d left", d2, game->remaining_counts[d2]);
      }
    } else {
      p2[0] = '\0';
    }

    snprintf(side[6 + pair], sizeof(side[6 + pair]), "%s│%s  %-12s   %-12s   %s│%s",
             COLOR_BOX_BORDER, COLOR_RESET, p1, p2, COLOR_BOX_BORDER, COLOR_RESET);
  }

  snprintf(side[11], sizeof(side[11]), "%s├── CONTROLS ───────────────────────┤%s", COLOR_BOX_BORDER, COLOR_RESET);
  snprintf(side[12], sizeof(side[12]), "%s│%s  Arrows / WASD : Navigate Grid   %s│%s", COLOR_BOX_BORDER, COLOR_RESET, COLOR_BOX_BORDER, COLOR_RESET);
  snprintf(side[13], sizeof(side[13]), "%s│%s  1 - 9         : Place / Note    %s│%s", COLOR_BOX_BORDER, COLOR_RESET, COLOR_BOX_BORDER, COLOR_RESET);
  snprintf(side[14], sizeof(side[14]), "%s│%s  N             : Note Pencil Mode%s│%s", COLOR_BOX_BORDER, COLOR_RESET, COLOR_BOX_BORDER, COLOR_RESET);
  snprintf(side[15], sizeof(side[15]), "%s│%s  0 / Backspace : Clear Cell      %s│%s", COLOR_BOX_BORDER, COLOR_RESET, COLOR_BOX_BORDER, COLOR_RESET);
  snprintf(side[16], sizeof(side[16]), "%s│%s  U             : Undo Move       %s│%s", COLOR_BOX_BORDER, COLOR_RESET, COLOR_BOX_BORDER, COLOR_RESET);
  snprintf(side[17], sizeof(side[17]), "%s│%s  H             : Hint (%d used)   %s│%s", COLOR_BOX_BORDER, COLOR_RESET, game->hints_used, COLOR_BOX_BORDER, COLOR_RESET);
  snprintf(side[18], sizeof(side[18]), "%s│%s  E             : Conflict Check  %s│%s", COLOR_BOX_BORDER, COLOR_RESET, COLOR_BOX_BORDER, COLOR_RESET);
  snprintf(side[19], sizeof(side[19]), "%s│%s  Tab / M       : Change Diff     %s│%s", COLOR_BOX_BORDER, COLOR_RESET, COLOR_BOX_BORDER, COLOR_RESET);
  snprintf(side[20], sizeof(side[20]), "%s│%s  P / Esc       : Pause           %s│%s", COLOR_BOX_BORDER, COLOR_RESET, COLOR_BOX_BORDER, COLOR_RESET);
  snprintf(side[21], sizeof(side[21]), "%s│%s  R             : New Game        %s│%s", COLOR_BOX_BORDER, COLOR_RESET, COLOR_BOX_BORDER, COLOR_RESET);
  snprintf(side[22], sizeof(side[22]), "%s│%s  Q             : Quit to Arcade  %s│%s", COLOR_BOX_BORDER, COLOR_RESET, COLOR_BOX_BORDER, COLOR_RESET);
  snprintf(side[23], sizeof(side[23]), "%s└───────────────────────────────────┘%s", COLOR_BOX_BORDER, COLOR_RESET);

  // Now render Board side-by-side with Sidebar
  int side_idx = 0;

  // Board Top Border
  offset += snprintf(
    buffer + offset, sizeof(buffer) - offset,
    "  %s╔═══╤═══╤═══╦═══╤═══╤═══╦═══╤═══╤═══╗%s   %s\n",
    COLOR_BOX_BORDER, COLOR_RESET, side[side_idx++]
  );

  for (int r = 0; r < GRID_SIZE; r++) {
    // Row content
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "  %s║%s", COLOR_BOX_BORDER, COLOR_RESET);

    for (int c = 0; c < GRID_SIZE; c++) {
      char cell_str[128];
      format_cell(game, r, c, sel_val, cell_str, sizeof(cell_str));
      offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s", cell_str);

      if (c == 2 || c == 5) {
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s║%s", COLOR_BOX_BORDER, COLOR_RESET);
      } else if (c == 8) {
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s║%s", COLOR_BOX_BORDER, COLOR_RESET);
      } else {
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s│%s", COLOR_CELL_BORDER, COLOR_RESET);
      }
    }

    // Append sidebar line
    const char *s_line = (side_idx < 25) ? side[side_idx++] : "";
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "   %s\n", s_line);

    // Row divider
    if (r == 2 || r == 5) {
      const char *sd = (side_idx < 25) ? side[side_idx++] : "";
      offset += snprintf(
        buffer + offset, sizeof(buffer) - offset,
        "  %s╠═══╪═══╪═══╬═══╪═══╪═══╬═══╪═══╪═══╣%s   %s\n",
        COLOR_BOX_BORDER, COLOR_RESET, sd
      );
    } else if (r != 8) {
      const char *sd = (side_idx < 25) ? side[side_idx++] : "";
      offset += snprintf(
        buffer + offset, sizeof(buffer) - offset,
        "  %s╟%s───%s┼%s───%s┼%s───%s╫%s───%s┼%s───%s┼%s───%s╫%s───%s┼%s───%s┼%s───%s╢%s   %s\n",
        COLOR_BOX_BORDER, COLOR_CELL_BORDER, COLOR_CELL_BORDER, COLOR_CELL_BORDER,
        COLOR_CELL_BORDER, COLOR_CELL_BORDER, COLOR_BOX_BORDER, COLOR_CELL_BORDER,
        COLOR_CELL_BORDER, COLOR_CELL_BORDER, COLOR_CELL_BORDER, COLOR_CELL_BORDER,
        COLOR_BOX_BORDER, COLOR_CELL_BORDER, COLOR_CELL_BORDER, COLOR_CELL_BORDER,
        COLOR_CELL_BORDER, COLOR_CELL_BORDER, COLOR_BOX_BORDER, COLOR_RESET, sd
      );
    }
  }

  // Board Bottom Border
  const char *sd_bottom = (side_idx < 25) ? side[side_idx++] : "";
  offset += snprintf(
    buffer + offset, sizeof(buffer) - offset,
    "  %s╚═══╧═══╧═══╩═══╧═══╧═══╩═══╧═══╧═══╝%s   %s\n",
    COLOR_BOX_BORDER, COLOR_RESET, sd_bottom
  );

  // Remaining sidebar lines if any
  while (side_idx < 24) {
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "                                              %s\n", side[side_idx++]);
  }

  // Overlays for Victory and Pause
  if (game->state == STATE_VICTORY) {
    offset += snprintf(
      buffer + offset, sizeof(buffer) - offset,
      "\n  \033[1;38;2;255;220;50;48;2;30;70;40m"
      " ★★★ VICTORY! SUDOKU SOLVED IN %02d:%02d! (Moves: %d, Mistakes: %d) [R to Play Again] ★★★ "
      "\033[0m\n",
      mins, secs, game->moves_made, game->mistakes
    );
  } else if (game->state == STATE_PAUSED) {
    offset += snprintf(
      buffer + offset, sizeof(buffer) - offset,
      "\n  \033[1;38;2;255;255;255;48;2;60;60;80m"
      "                   [ GAME PAUSED - Press P or Esc to Resume ]                   "
      "\033[0m\n"
    );
  } else {
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "\n");
  }

  printf("%s", buffer);
  fflush(stdout);
}
