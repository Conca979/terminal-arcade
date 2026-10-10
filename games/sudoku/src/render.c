#include "render.h"
#include <stdio.h>
#include <string.h>

#define COLOR_RESET "\033[0m"
#define COLOR_BOX_BORDER "\033[1;38;2;90;140;220m"
#define COLOR_CELL_BORDER "\033[38;2;80;110;150m"
#define COLOR_TITLE "\033[1;38;2;255;255;255m"
#define COLOR_LABEL "\033[1;38;2;160;185;220m"
#define COLOR_VALUE "\033[1;38;2;255;220;50m"
#define COLOR_ACCENT "\033[1;38;2;0;225;225m"
#define COLOR_DIM "\033[38;2;120;135;160m"
#define COLOR_DONE "\033[1;38;2;80;220;100m"

// Cell formatting
#define COLOR_GIVEN "\033[1;38;2;245;245;255m"
#define COLOR_USER "\033[1;38;2;0;230;240m"
#define COLOR_NOTE "\033[1;38;2;245;190;60m"

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
        snprintf(out, out_sz, "%s%s * %s", bg, COLOR_NOTE, COLOR_RESET);
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

static void format_digit_stat(int digit, int rem, char *txt, size_t txt_sz, const char **col) {
  if (rem == 0) {
    snprintf(txt, txt_sz, "%d: DONE  ", digit);
    *col = COLOR_DONE;
  } else {
    snprintf(txt, txt_sz, "%d: %d left", digit, rem);
    *col = COLOR_VALUE;
  }
}

void render_frame(const GameState *game, unsigned long long current_time) {
  static char buffer[32768];
  int offset = 0;

  offset += snprintf(buffer + offset, sizeof(buffer) - offset, "\033[H");

  // Title Banner - exactly 76 characters wide (matching board + sidebar width)
  offset += snprintf(
    buffer + offset, sizeof(buffer) - offset,
    "  %s╔══════════════════════════════════════════════════════════════════════════╗%s\n"
    "  %s║%s                        %sS U D O K U   A R C A D E%s                         %s║%s\n"
    "  %s╚══════════════════════════════════════════════════════════════════════════╝%s\n",
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

  // Prepare exactly 19 sidebar lines, each with exactly 34 columns of inside content
  char side[19][256];

  // Line 0: Inspector Header
  snprintf(side[0], sizeof(side[0]), "%s┌── CELL INSPECTOR ────────────────┐%s", COLOR_BOX_BORDER, COLOR_RESET);

  // Line 1: Coordinates (1 + 20 + 13 = 34 cols)
  snprintf(side[1], sizeof(side[1]), "%s│%s Pos:   Row %d, Col %d              %s│%s",
           COLOR_BOX_BORDER, COLOR_RESET, game->cursor_r + 1, game->cursor_c + 1, COLOR_BOX_BORDER, COLOR_RESET);

  // Line 2: Value status (34 cols inside)
  if (sel_val > 0) {
    if (is_cur_given) {
      snprintf(side[2], sizeof(side[2]), "%s│%s Val:   %s%d [GIVEN CLUE]%s            %s│%s",
               COLOR_BOX_BORDER, COLOR_RESET, COLOR_GIVEN, sel_val, COLOR_RESET, COLOR_BOX_BORDER, COLOR_RESET);
    } else {
      snprintf(side[2], sizeof(side[2]), "%s│%s Val:   %s%d [USER VALUE]%s            %s│%s",
               COLOR_BOX_BORDER, COLOR_RESET, COLOR_USER, sel_val, COLOR_RESET, COLOR_BOX_BORDER, COLOR_RESET);
    }
  } else {
    snprintf(side[2], sizeof(side[2]), "%s│%s Val:   \033[38;2;120;130;140m(Empty)\033[0m                    %s│%s",
             COLOR_BOX_BORDER, COLOR_RESET, COLOR_BOX_BORDER, COLOR_RESET);
  }

  // Line 3: Candidate Notes (8 + 24 + 2 = 34 cols inside)
  char notes_buf[32] = "";
  int nb_len = 0;
  for (int n = 1; n <= 9; n++) {
    if (cursor_notes & (1 << n)) {
      nb_len += snprintf(notes_buf + nb_len, sizeof(notes_buf) - nb_len, "%d ", n);
    }
  }
  if (nb_len == 0) snprintf(notes_buf, sizeof(notes_buf), "None");
  snprintf(side[3], sizeof(side[3]), "%s│%s Notes: %s%-24s%s  %s│%s",
           COLOR_BOX_BORDER, COLOR_RESET, COLOR_NOTE, notes_buf, COLOR_RESET, COLOR_BOX_BORDER, COLOR_RESET);

  // Line 4: Valid Candidates (8 + 24 + 2 = 34 cols inside)
  char cand_buf[32] = "";
  int cb_len = 0;
  if (sel_val == 0) {
    for (int n = 1; n <= 9; n++) {
      if (candidates & (1 << n)) {
        cb_len += snprintf(cand_buf + cb_len, sizeof(cand_buf) - cb_len, "%d ", n);
      }
    }
    if (cb_len == 0) snprintf(cand_buf, sizeof(cand_buf), "None (Conflict!)");
  } else {
    snprintf(cand_buf, sizeof(cand_buf), "(Cell is filled)");
  }
  snprintf(side[4], sizeof(side[4]), "%s│%s Cand:  \033[1;36m%-24s\033[0m  %s│%s",
           COLOR_BOX_BORDER, COLOR_RESET, cand_buf, COLOR_BOX_BORDER, COLOR_RESET);

  // Line 5: Remaining Digits Section Divider
  snprintf(side[5], sizeof(side[5]), "%s├── REMAINING DIGITS ──────────────┤%s", COLOR_BOX_BORDER, COLOR_RESET);

  // Lines 6, 7, 8: Digits 1..9 (2 + 9 + 2 + 9 + 2 + 9 + 1 = 34 cols inside)
  for (int row = 0; row < 3; row++) {
    int d1 = row * 3 + 1;
    int d2 = row * 3 + 2;
    int d3 = row * 3 + 3;

    char t1[16], t2[16], t3[16];
    const char *c1;
    const char *c2;
    const char *c3;

    format_digit_stat(d1, game->remaining_counts[d1], t1, sizeof(t1), &c1);
    format_digit_stat(d2, game->remaining_counts[d2], t2, sizeof(t2), &c2);
    format_digit_stat(d3, game->remaining_counts[d3], t3, sizeof(t3), &c3);

    snprintf(side[6 + row], sizeof(side[6 + row]),
             "%s│%s  %s%s%s  %s%s%s  %s%s%s %s│%s",
             COLOR_BOX_BORDER, COLOR_RESET,
             c1, t1, COLOR_RESET,
             c2, t2, COLOR_RESET,
             c3, t3, COLOR_RESET,
             COLOR_BOX_BORDER, COLOR_RESET);
  }

  // Line 9: Controls Section Divider
  snprintf(side[9], sizeof(side[9]), "%s├── CONTROLS ──────────────────────┤%s", COLOR_BOX_BORDER, COLOR_RESET);

  // Lines 10-17: Keybindings (exactly 34 cols inside each)
  snprintf(side[10], sizeof(side[10]), "%s│%s  Arrows / WASD : Navigate Grid   %s│%s", COLOR_BOX_BORDER, COLOR_RESET, COLOR_BOX_BORDER, COLOR_RESET);
  snprintf(side[11], sizeof(side[11]), "%s│%s  1 - 9         : Number / Note   %s│%s", COLOR_BOX_BORDER, COLOR_RESET, COLOR_BOX_BORDER, COLOR_RESET);
  snprintf(side[12], sizeof(side[12]), "%s│%s  N             : Note Pencil Mode%s│%s", COLOR_BOX_BORDER, COLOR_RESET, COLOR_BOX_BORDER, COLOR_RESET);
  snprintf(side[13], sizeof(side[13]), "%s│%s  0 / Backspace : Clear Cell      %s│%s", COLOR_BOX_BORDER, COLOR_RESET, COLOR_BOX_BORDER, COLOR_RESET);
  snprintf(side[14], sizeof(side[14]), "%s│%s  U / Z         : Undo Move       %s│%s", COLOR_BOX_BORDER, COLOR_RESET, COLOR_BOX_BORDER, COLOR_RESET);

  char hint_buf[35];
  snprintf(hint_buf, sizeof(hint_buf), "  H             : Hint (%d used)", game->hints_used);
  snprintf(side[15], sizeof(side[15]), "%s│%s%-34s%s│%s", COLOR_BOX_BORDER, COLOR_RESET, hint_buf, COLOR_BOX_BORDER, COLOR_RESET);

  snprintf(side[16], sizeof(side[16]), "%s│%s  E             : Conflict Check  %s│%s", COLOR_BOX_BORDER, COLOR_RESET, COLOR_BOX_BORDER, COLOR_RESET);
  snprintf(side[17], sizeof(side[17]), "%s│%s  Tab: Diff  P: Pause  Q: Quit    %s│%s", COLOR_BOX_BORDER, COLOR_RESET, COLOR_BOX_BORDER, COLOR_RESET);

  // Line 18: Sidebar Bottom Border
  snprintf(side[18], sizeof(side[18]), "%s└──────────────────────────────────┘%s", COLOR_BOX_BORDER, COLOR_RESET);

  // Render 19 lines of Board side-by-side with 19 lines of Sidebar
  int side_idx = 0;

  // Board Top Border (Line 0)
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

      if (c == 2 || c == 5 || c == 8) {
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s║%s", COLOR_BOX_BORDER, COLOR_RESET);
      } else {
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s│%s", COLOR_CELL_BORDER, COLOR_RESET);
      }
    }

    // Append corresponding sidebar line
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "   %s\n", side[side_idx++]);

    // Row divider after row r (except after row 8)
    if (r == 2 || r == 5) {
      // 3x3 block horizontal boundary
      offset += snprintf(
        buffer + offset, sizeof(buffer) - offset,
        "  %s╠═══╪═══╪═══╬═══╪═══╪═══╬═══╪═══╪═══╣%s   %s\n",
        COLOR_BOX_BORDER, COLOR_RESET, side[side_idx++]
      );
    } else if (r != 8) {
      // Inner cell horizontal divider
      offset += snprintf(
        buffer + offset, sizeof(buffer) - offset,
        "  %s╟%s───┼───┼───%s╫%s───┼───┼───%s╫%s───┼───┼───%s╢%s   %s\n",
        COLOR_BOX_BORDER, COLOR_CELL_BORDER,
        COLOR_BOX_BORDER, COLOR_CELL_BORDER,
        COLOR_BOX_BORDER, COLOR_CELL_BORDER,
        COLOR_BOX_BORDER, COLOR_RESET,
        side[side_idx++]
      );
    }
  }

  // Board Bottom Border (Line 18)
  offset += snprintf(
    buffer + offset, sizeof(buffer) - offset,
    "  %s╚═══╧═══╧═══╩═══╧═══╧═══╩═══╧═══╧═══╝%s   %s\n",
    COLOR_BOX_BORDER, COLOR_RESET, side[side_idx++]
  );

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
