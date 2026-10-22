#include "render.h"
#include <stdio.h>
#include <string.h>

#define COLOR_RESET       "\033[0m"
#define COLOR_BORDER      "\033[38;2;90;145;200m"
#define COLOR_GRID        "\033[38;2;110;125;145m"
#define COLOR_TITLE       "\033[1;38;2;255;255;255m"
#define COLOR_GOLD        "\033[1;38;2;255;215;70m"
#define COLOR_CYAN        "\033[1;38;2;0;230;225m"
#define COLOR_DIM         "\033[38;2;140;155;175m"
#define COLOR_ALERT       "\033[1;38;2;255;85;85m"
#define COLOR_WIN         "\033[1;38;2;80;255;130m"
#define COLOR_SNAP        "\033[1;38;2;255;230;80m"
#define COLOR_HINT        "\033[1;38;2;95;255;145m"

#define BG_EMPTY_SLOT_1   "\033[48;2;32;36;46m"
#define BG_EMPTY_SLOT_2   "\033[48;2;24;28;38m"
#define BG_CURSOR_BOARD   "\033[48;2;30;110;150m"
#define BG_HINT_SLOT      "\033[48;2;40;130;70m"

#define LEFT_PANEL_WIDTH  40
#define RIGHT_PANEL_WIDTH 51
#define TABLE_BORDER_HORIZ "════════════════════════════════════════════════════════════════════════════════════════════"

void render_init(void) {
  printf("\033[2J\033[H");
  fflush(stdout);
}

void render_cleanup(void) {
  printf("%s", COLOR_RESET);
  fflush(stdout);
}

static int visible_length(const char *s) {
  if (!s) return 0;
  int len = 0;
  bool in_escape = false;
  for (int i = 0; s[i] != '\0'; i++) {
    if (s[i] == '\033') {
      in_escape = true;
    } else if (in_escape) {
      if ((s[i] >= 'A' && s[i] <= 'Z') || (s[i] >= 'a' && s[i] <= 'z')) {
        in_escape = false;
      }
    } else {
      unsigned char c = (unsigned char)s[i];
      if ((c & 0xC0) != 0x80) {
        len++;
      }
    }
  }
  return len;
}

static void print_table_line(const char *content) {
  int vis = visible_length(content);
  int pad = INNER_WIDTH - vis;
  if (pad < 0) pad = 0;
  printf("  %s║%s%s%*s%s║%s\033[K\n",
         COLOR_BORDER, COLOR_RESET, content, pad, "", COLOR_BORDER, COLOR_RESET);
}

static void print_split_line(const char *left_col, const char *right_col) {
  int vis_l = visible_length(left_col);
  int pad_l = LEFT_PANEL_WIDTH - vis_l;
  if (pad_l < 0) pad_l = 0;

  int vis_r = visible_length(right_col);
  int pad_r = RIGHT_PANEL_WIDTH - vis_r;
  if (pad_r < 0) pad_r = 0;

  printf("  %s║%s%s%*s%s│%s%s%*s%s║%s\033[K\n",
         COLOR_BORDER, COLOR_RESET,
         left_col, pad_l, "",
         COLOR_BORDER, COLOR_RESET,
         right_col, pad_r, "",
         COLOR_BORDER, COLOR_RESET);
}

bool render_get_board_slot_at_pos(int mouse_x, int mouse_y, int *out_r, int *out_c) {
  /* Board starts at mouse_x = 7..38 (4 cols * 8 chars) and mouse_y = 5..16 (4 rows * 3 chars) */
  if (mouse_x < 7 || mouse_x > 38 || mouse_y < 5 || mouse_y > 16) {
    return false;
  }
  int c = (mouse_x - 7) / TILE_WIDTH;
  int r = (mouse_y - 5) / TILE_HEIGHT;
  if (r >= 0 && r < GRID_SIZE && c >= 0 && c < GRID_SIZE) {
    if (out_r) *out_r = r;
    if (out_c) *out_c = c;
    return true;
  }
  return false;
}

bool render_get_tray_slot_at_pos(int mouse_x, int mouse_y, int *out_idx) {
  /* Tray slots are rendered in the right panel rows 10..17, cols 46..89 */
  /* 4 rows of 4 tray pieces: mouse_y in 10..17 (2 rows per tray rank), mouse_x in 46..89 (11 cols per slot) */
  if (mouse_x < 45 || mouse_x > 88 || mouse_y < 10 || mouse_y > 17) {
    return false;
  }
  int tr = (mouse_y - 10) / 2;
  int tc = (mouse_x - 45) / 11;
  if (tr >= 0 && tr < 4 && tc >= 0 && tc < 4) {
    int idx = tr * 4 + tc;
    if (out_idx) *out_idx = idx;
    return true;
  }
  return false;
}

static void format_board_tile_slice(const GameState *g, const JigsawTheme *theme,
                                    int r, int c, int sub_r, char *out, size_t size) {
  int piece_id = g->board[r][c];
  bool is_cur = (g->focus == FOCUS_BOARD && g->cursor_board_r == r && g->cursor_board_c == c);
  bool is_hint_target = (g->hint_target_r == r && g->hint_target_c == c);
  bool is_snapped = (piece_id == (r * GRID_SIZE + c));

  if (piece_id != -1) {
    /* Render slice from theme canvas */
    int pr = piece_id / GRID_SIZE;
    int pc = piece_id % GRID_SIZE;
    int canvas_r = pr * TILE_HEIGHT + sub_r;
    int canvas_c = pc * TILE_WIDTH;

    int pos = 0;
    for (int sc = 0; sc < TILE_WIDTH; sc++) {
      const PixelCell *cell = &theme->canvas[canvas_r][canvas_c + sc];
      const char *bg = cell->bg_color;

      if (is_cur && (sub_r == 0 || sub_r == TILE_HEIGHT - 1 || sc == 0 || sc == TILE_WIDTH - 1)) {
        bg = BG_CURSOR_BOARD;
      } else if (is_hint_target && (sub_r == 0 || sub_r == TILE_HEIGHT - 1)) {
        bg = BG_HINT_SLOT;
      }

      /* Snapped indicator corner tab on top-right of tile */
      if (is_snapped && sub_r == 0 && sc == TILE_WIDTH - 1) {
        pos += snprintf(out + pos, size - pos, "%s%s✔%s", bg, COLOR_SNAP, COLOR_RESET);
      } else {
        pos += snprintf(out + pos, size - pos, "%s%s%s%s", bg, cell->fg_color, cell->glyph, COLOR_RESET);
      }
    }
  } else {
    /* Empty slot */
    const char *bg = ((r + c) % 2 == 0) ? BG_EMPTY_SLOT_1 : BG_EMPTY_SLOT_2;
    if (is_cur) bg = BG_CURSOR_BOARD;
    else if (is_hint_target) bg = BG_HINT_SLOT;

    if (sub_r == 0) {
      snprintf(out, size, "%s%s ┌────┐ %s", bg, is_cur ? COLOR_CYAN : COLOR_GRID, COLOR_RESET);
    } else if (sub_r == 1) {
      char coord[8];
      snprintf(coord, sizeof(coord), "%c%d", 'A' + c, r + 1);
      snprintf(out, size, "%s%s │ %s │ %s", bg, is_cur ? COLOR_CYAN : COLOR_GRID, coord, COLOR_RESET);
    } else {
      snprintf(out, size, "%s%s └────┘ %s", bg, is_cur ? COLOR_CYAN : COLOR_GRID, COLOR_RESET);
    }
  }
}

static void format_right_panel_row(const GameState *g, const JigsawTheme *theme,
                                   int row_idx, char *out, size_t size) {
  out[0] = '\0';

  if (row_idx == 0) {
    snprintf(out, size, " %sJIGSAW SNAP DASHBOARD%s  Theme: %s%s%s",
             COLOR_GOLD, COLOR_RESET, COLOR_CYAN, theme->title, COLOR_RESET);
  } else if (row_idx == 1) {
    int pct = (g->snapped_count * 100) / TOTAL_PIECES;
    snprintf(out, size, " Genre: %s%-16s%s Progress: %s%2d/%d (%d%%)%s",
             COLOR_DIM, theme->category, COLOR_RESET,
             (g->snapped_count == TOTAL_PIECES) ? COLOR_WIN : COLOR_GOLD,
             g->snapped_count, TOTAL_PIECES, pct, COLOR_RESET);
  } else if (row_idx == 2) {
    /* Progress bar */
    char bar[17];
    int filled = (g->snapped_count * 16) / TOTAL_PIECES;
    for (int i = 0; i < 16; i++) {
      bar[i] = (i < filled) ? '=' : '-';
    }
    bar[16] = '\0';
    int sec = (int)g->elapsed_seconds;
    snprintf(out, size, " [%s%s%s] Moves: %s%-3d%s Time: %02d:%02d",
             COLOR_WIN, bar, COLOR_RESET,
             COLOR_CYAN, g->moves_count, COLOR_RESET,
             sec / 60, sec % 60);
  } else if (row_idx == 3) {
    if (g->holding_piece != -1) {
      snprintf(out, size, " IN HAND: %s[Piece #%02d]%s (Click slot or press Space)",
               COLOR_GOLD, g->holding_piece + 1, COLOR_RESET);
    } else {
      snprintf(out, size, " IN HAND: %s(None)%s        (Click piece or Tab->Tray)",
               COLOR_DIM, COLOR_RESET);
    }
  } else if (row_idx == 4) {
    snprintf(out, size, " %s─────────────────────────────────────────────────%s", COLOR_GRID, COLOR_RESET);
  } else if (g->show_preview) {
    /* Artwork Preview Mode: display master reference */
    if (row_idx == 5) {
      snprintf(out, size, " %sFULL ARTWORK REFERENCE GUIDE%s  [P] Hide Preview",
               COLOR_GOLD, COLOR_RESET);
    } else if (row_idx >= 6 && row_idx < 18) {
      int art_r = row_idx - 6;
      int pos = snprintf(out, size, "   ");
      for (int c = 0; c < ART_WIDTH; c++) {
        const PixelCell *cell = &theme->canvas[art_r][c];
        pos += snprintf(out + pos, size - pos, "%s%s%s%s",
                        cell->bg_color, cell->fg_color, cell->glyph, COLOR_RESET);
      }
    }
  } else {
    /* Tray / Pool Mode */
    if (row_idx == 5) {
      const char *focus_badge = (g->focus == FOCUS_TRAY)
        ? "\033[1;38;2;0;230;225m[ACTIVE FOCUS]\033[0m"
        : "\033[38;2;140;155;175m[Press Tab to Focus]\033[0m";
      snprintf(out, size, " %sPIECE TRAY (Unplaced Chunks)%s  %s",
               COLOR_GOLD, COLOR_RESET, focus_badge);
    } else if (row_idx >= 6 && row_idx < 14) {
      int tray_row = (row_idx - 6) / 2;
      int sub_tr = (row_idx - 6) % 2;

      int pos = snprintf(out, size, " ");
      for (int tray_col = 0; tray_col < 4; tray_col++) {
        int tray_idx = tray_row * 4 + tray_col;
        int piece_id = g->tray_order[tray_idx];
        bool is_present = g->tray_present[piece_id];
        bool is_tray_cur = (g->focus == FOCUS_TRAY && g->cursor_tray_idx == tray_idx);
        bool is_hint_p = (g->hint_piece_id == piece_id);
        bool is_holding_this = (g->holding_piece == piece_id);

        if (sub_tr == 0) {
          if (is_present) {
            const char *bracket_l = is_tray_cur ? "►" : "[";
            const char *bracket_r = is_tray_cur ? "◄" : "]";
            const char *col = is_hint_p ? COLOR_HINT : (is_tray_cur ? COLOR_CYAN : COLOR_TITLE);
            pos += snprintf(out + pos, size - pos, " %s%s#%02d%s%s   ",
                            col, bracket_l, piece_id + 1, bracket_r, COLOR_RESET);
          } else if (is_holding_this) {
            pos += snprintf(out + pos, size - pos, " %s*HOLD*%-2s%s   ",
                            COLOR_GOLD, "", COLOR_RESET);
          } else {
            pos += snprintf(out + pos, size - pos, " %s --   -- %s   ",
                            COLOR_DIM, COLOR_RESET);
          }
        } else {
          if (is_present) {
            /* Show miniature 5-char pixel preview from the piece's center */
            int pr = piece_id / GRID_SIZE;
            int pc = piece_id % GRID_SIZE;
            int cr = pr * TILE_HEIGHT + 1;
            int cc = pc * TILE_WIDTH + 1;
            pos += snprintf(out + pos, size - pos, "  ");
            for (int k = 0; k < 6; k++) {
              const PixelCell *cell = &theme->canvas[cr][cc + k];
              pos += snprintf(out + pos, size - pos, "%s%s%s%s",
                              cell->bg_color, cell->fg_color, cell->glyph, COLOR_RESET);
            }
            pos += snprintf(out + pos, size - pos, "   ");
          } else {
            pos += snprintf(out + pos, size - pos, " %s  (placed) %s  ",
                            COLOR_DIM, COLOR_RESET);
          }
        }
      }
    } else if (row_idx == 14) {
      snprintf(out, size, " %s─────────────────────────────────────────────────%s", COLOR_GRID, COLOR_RESET);
    } else if (row_idx == 15) {
      snprintf(out, size, " %s[WASD/Mouse]%s Move/Select  %s[Space]%s Pick / Place",
               COLOR_CYAN, COLOR_RESET, COLOR_GOLD, COLOR_RESET);
    } else if (row_idx == 16) {
      snprintf(out, size, " %s[X]%s Return Piece  %s[Tab]%s Board/Tray  %s[H]%s Hint",
               COLOR_GOLD, COLOR_RESET, COLOR_CYAN, COLOR_RESET, COLOR_GOLD, COLOR_RESET);
    } else if (row_idx == 17) {
      snprintf(out, size, " %s[T]%s Next Theme    %s[P]%s Art Preview   %s[U]%s Undo",
               COLOR_CYAN, COLOR_RESET, COLOR_GOLD, COLOR_RESET, COLOR_CYAN, COLOR_RESET);
    }
  }
}

void render_game(const GameState *g) {
  printf("\033[H");
  const JigsawTheme *theme = jigsaw_get_theme(g->current_theme);

  /* Line 1: Top outer border */
  printf("  %s╔" TABLE_BORDER_HORIZ "╗%s\033[K\n", COLOR_BORDER, COLOR_RESET);

  /* Line 2: Header bar */
  char header[512];
  int pct = (g->snapped_count * 100) / TOTAL_PIECES;
  int sec = (int)g->elapsed_seconds;
  snprintf(header, sizeof(header),
           " %s🧩 JIGSAW SNAP CLI%s │ %sTHEME: %s%-14s%s │ %sSNAPPED: %2d/16 (%d%%)%s │ %sMOVES: %-3d%s │ %02d:%02d",
           COLOR_TITLE, COLOR_RESET,
           COLOR_GOLD, COLOR_CYAN, theme->title, COLOR_RESET,
           (g->snapped_count == TOTAL_PIECES) ? COLOR_WIN : COLOR_GOLD,
           g->snapped_count, pct, COLOR_RESET,
           COLOR_CYAN, g->moves_count, COLOR_RESET,
           sec / 60, sec % 60);
  print_table_line(header);

  /* Line 3: Top separator */
  printf("  %s╠" TABLE_BORDER_HORIZ "╣%s\033[K\n", COLOR_BORDER, COLOR_RESET);

  /* Lines 4..21 (18 rows): Board (Left 40 cols) + Divider + Right Panel (51 cols) */
  for (int row = 0; row < 18; row++) {
    char left_buf[4096];
    char right_buf[4096];

    if (row == 0) {
      snprintf(left_buf, sizeof(left_buf), "      %sA       B       C       D%s     ",
               COLOR_DIM, COLOR_RESET);
    } else if (row == 1) {
      snprintf(left_buf, sizeof(left_buf), "   %s┌────────────────────────────────┐%s   ",
               COLOR_GRID, COLOR_RESET);
    } else if (row >= 2 && row <= 13) {
      int board_row = (row - 2) / TILE_HEIGHT;
      int sub_r = (row - 2) % TILE_HEIGHT;
      int rank_num = board_row + 1;

      int pos = 0;
      if (sub_r == 1) {
        pos += snprintf(left_buf + pos, sizeof(left_buf) - pos, " %s%d%s %s│%s",
                        COLOR_DIM, rank_num, COLOR_RESET, COLOR_GRID, COLOR_RESET);
      } else {
        pos += snprintf(left_buf + pos, sizeof(left_buf) - pos, "   %s│%s",
                        COLOR_GRID, COLOR_RESET);
      }

      for (int board_col = 0; board_col < GRID_SIZE; board_col++) {
        char tile_slice[256];
        format_board_tile_slice(g, theme, board_row, board_col, sub_r, tile_slice, sizeof(tile_slice));
        pos += snprintf(left_buf + pos, sizeof(left_buf) - pos, "%s", tile_slice);
      }

      if (sub_r == 1) {
        snprintf(left_buf + pos, sizeof(left_buf) - pos, "%s│%s %s%d%s ",
                 COLOR_GRID, COLOR_RESET, COLOR_DIM, rank_num, COLOR_RESET);
      } else {
        snprintf(left_buf + pos, sizeof(left_buf) - pos, "%s│%s   ",
                 COLOR_GRID, COLOR_RESET);
      }
    } else if (row == 14) {
      snprintf(left_buf, sizeof(left_buf), "   %s└────────────────────────────────┘%s   ",
               COLOR_GRID, COLOR_RESET);
    } else if (row == 15) {
      snprintf(left_buf, sizeof(left_buf), "      %sA       B       C       D%s     ",
               COLOR_DIM, COLOR_RESET);
    } else {
      snprintf(left_buf, sizeof(left_buf), "                                        ");
    }

    format_right_panel_row(g, theme, row, right_buf, sizeof(right_buf));
    print_split_line(left_buf, right_buf);
  }

  /* Line 22: Bottom separator */
  printf("  %s╠" TABLE_BORDER_HORIZ "╣%s\033[K\n", COLOR_BORDER, COLOR_RESET);

  /* Line 23: Status bar */
  char status[512];
  const char *msg_col = g->is_solved ? COLOR_WIN : COLOR_TITLE;
  snprintf(status, sizeof(status), " %sSTATUS:%s %s%s%s",
           COLOR_GOLD, COLOR_RESET, msg_col, g->status_msg, COLOR_RESET);
  print_table_line(status);

  /* Line 24: Controls bar */
  char controls[512];
  snprintf(controls, sizeof(controls),
           " %s[Mouse/WASD]%s Move  %s[Space]%s Place  %s[X]%s Return  %s[H]%s Hint  %s[T]%s Theme  %s[P]%s Art  %s[Q]%s Quit",
           COLOR_CYAN, COLOR_RESET,
           COLOR_GOLD, COLOR_RESET,
           COLOR_CYAN, COLOR_RESET,
           COLOR_GOLD, COLOR_RESET,
           COLOR_CYAN, COLOR_RESET,
           COLOR_GOLD, COLOR_RESET,
           COLOR_CYAN, COLOR_RESET);
  print_table_line(controls);

  /* Line 25: Bottom outer border */
  printf("  %s╚" TABLE_BORDER_HORIZ "╝%s\033[K\n", COLOR_BORDER, COLOR_RESET);

  fflush(stdout);
}
