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

/* Square background & piece foreground colors */
#define BG_LIGHT_SQ       "\033[48;2;72;82;98m"
#define BG_DARK_SQ        "\033[48;2;42;48;60m"
#define BG_SELECTED       "\033[48;2;165;130;30m"
#define BG_CURSOR         "\033[48;2;30;115;145m"
#define BG_LAST_MOVE      "\033[48;2;65;90;65m"
#define BG_TARGET_CAP     "\033[48;2;135;45;55m"
#define BG_CHECK_KING     "\033[48;2;175;35;45m"

#define FG_WHITE_PIECE    "\033[1;38;2;255;250;225m"
#define FG_BLACK_PIECE    "\033[1;38;2;255;135;105m"
#define FG_TARGET_DOT     "\033[1;38;2;95;255;145m"

#define TABLE_BORDER_HORIZ "══════════════════════════════════════════════════════════════════════════════════════════════"

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

static void print_split_line(const char *left_55, const char *right_38) {
  int vis_l = visible_length(left_55);
  int pad_l = 55 - vis_l;
  if (pad_l < 0) pad_l = 0;

  int vis_r = visible_length(right_38);
  int pad_r = 38 - vis_r;
  if (pad_r < 0) pad_r = 0;

  printf("  %s║%s%s%*s%s│%s%s%*s%s║%s\033[K\n",
         COLOR_BORDER, COLOR_RESET,
         left_55, pad_l, "",
         COLOR_BORDER, COLOR_RESET,
         right_38, pad_r, "",
         COLOR_BORDER, COLOR_RESET);
}

static bool is_legal_target_from_selected(const GameState *g, int r, int c, bool *is_capture) {
  if (g->selected_r < 0 || g->selected_c < 0) return false;
  for (int i = 0; i < g->legal_move_count; i++) {
    ChessMove lm = g->legal_moves[i];
    if (lm.from_r == g->selected_r && lm.from_c == g->selected_c &&
        lm.to_r == r && lm.to_c == c) {
      if (is_capture) {
        *is_capture = (lm.captured.type != PIECE_NONE || lm.is_en_passant);
      }
      return true;
    }
  }
  return false;
}

bool render_get_square_at_pos(const GameState *g, int mouse_x, int mouse_y, int *out_r, int *out_c) {
  if (mouse_x < 7 || mouse_x > 54 || mouse_y < 4 || mouse_y > 20) {
    return false;
  }

  int display_col = (mouse_x - 7) / 6;
  int display_rank = (mouse_y - 4) / 2;
  if (display_col < 0 || display_col >= 8 || display_rank < 0 || display_rank >= 8) {
    return false;
  }

  bool flipped = (g != NULL) ? g->board_flipped : false;
  if (out_r) *out_r = flipped ? (7 - display_rank) : display_rank;
  if (out_c) *out_c = flipped ? (7 - display_col) : display_col;
  return true;
}

static void format_cell_5chars(const GameState *g, int r, int c, char *out, size_t size) {
  Piece p = g->board[r][c];
  bool is_sel = (g->selected_r == r && g->selected_c == c);
  bool is_cur = (g->cursor_r == r && g->cursor_c == c);
  bool is_last = g->has_last_move &&
                 ((g->last_move.from_r == r && g->last_move.from_c == c) ||
                  (g->last_move.to_r == r && g->last_move.to_c == c));
  bool is_cap_target = false;
  bool is_target = is_legal_target_from_selected(g, r, c, &is_cap_target);
  bool is_checked_king = (p.type == PIECE_KING && game_is_in_check(g, p.color));

  const char *bg = ((r + c) % 2 == 0) ? BG_LIGHT_SQ : BG_DARK_SQ;
  if (is_last) bg = BG_LAST_MOVE;
  if (is_checked_king) bg = BG_CHECK_KING;
  if (is_target && is_cap_target) bg = BG_TARGET_CAP;
  if (is_sel) bg = BG_SELECTED;
  if (is_cur) bg = BG_CURSOR;

  if (p.type != PIECE_NONE) {
    const char *fg = (p.color == COLOR_WHITE) ? FG_WHITE_PIECE : FG_BLACK_PIECE;
    const char *sym = game_piece_symbol(p);
    char let = game_piece_letter(p.type);
    if (is_target && is_cap_target) {
      snprintf(out, size, "%s%s[%s %c]%s", bg, fg, sym, let, COLOR_RESET);
    } else {
      snprintf(out, size, "%s%s %s %c %s", bg, fg, sym, let, COLOR_RESET);
    }
  } else {
    if (is_target) {
      snprintf(out, size, "%s%s  •  %s", bg, FG_TARGET_DOT, COLOR_RESET);
    } else {
      snprintf(out, size, "%s     %s", bg, COLOR_RESET);
    }
  }
}

static void format_captured_line(const PieceType *caps, int count, PieceColor color_of_captured,
                                 char *out, size_t size) {
  out[0] = '\0';
  int max_show = (count > 12) ? 12 : count;
  int pos = snprintf(out, size, "  %s", (color_of_captured == COLOR_WHITE) ? FG_WHITE_PIECE : FG_BLACK_PIECE);
  for (int i = 0; i < max_show; i++) {
    Piece p = { caps[i], color_of_captured };
    pos += snprintf(out + pos, size - pos, "%s ", game_piece_symbol(p));
  }
  if (count == 0) {
    pos += snprintf(out + pos, size - pos, "%s(none)", COLOR_DIM);
  }
  snprintf(out + pos, size - pos, "%s", COLOR_RESET);
}

static void format_sidebar_row(const GameState *g, int row_idx, char *out, size_t size) {
  out[0] = '\0';
  int mat_adv = game_material_advantage_white(g);

  switch (row_idx) {
    case 0:
      snprintf(out, size, " %sMATCH DASHBOARD%s", COLOR_GOLD, COLOR_RESET);
      break;
    case 1: {
      const char *turn_col = (g->turn == COLOR_WHITE) ? FG_WHITE_PIECE : FG_BLACK_PIECE;
      const char *turn_str = (g->turn == COLOR_WHITE) ? "WHITE (♔)" : "BLACK (♚)";
      snprintf(out, size, " Turn: %s%s%s", turn_col, turn_str, COLOR_RESET);
      break;
    }
    case 2:
      if (game_is_in_check(g, g->turn)) {
        snprintf(out, size, " State: %s⚠ IN CHECK!%s", COLOR_ALERT, COLOR_RESET);
      } else if (g->status == STATUS_CHECKMATE) {
        snprintf(out, size, " State: %s★ CHECKMATE ★%s", COLOR_WIN, COLOR_RESET);
      } else if (g->status != STATUS_PLAYING) {
        snprintf(out, size, " State: %sDRAW GAME%s", COLOR_GOLD, COLOR_RESET);
      } else {
        snprintf(out, size, " State: %sIn Progress (Move %d)%s", COLOR_CYAN, g->fullmove_number, COLOR_RESET);
      }
      break;
    case 3:
      if (mat_adv > 0) {
        snprintf(out, size, " Material: %sWhite +%d%s", FG_WHITE_PIECE, mat_adv, COLOR_RESET);
      } else if (mat_adv < 0) {
        snprintf(out, size, " Material: %sBlack +%d%s", FG_BLACK_PIECE, -mat_adv, COLOR_RESET);
      } else {
        snprintf(out, size, " Material: %sEven (0)%s", COLOR_DIM, COLOR_RESET);
      }
      break;
    case 4:
      snprintf(out, size, " %sCaptured by White:%s", COLOR_DIM, COLOR_RESET);
      break;
    case 5:
      format_captured_line(g->captured_by_white, g->cap_white_count, COLOR_BLACK, out, size);
      break;
    case 6:
      snprintf(out, size, " %sCaptured by Black:%s", COLOR_DIM, COLOR_RESET);
      break;
    case 7:
      format_captured_line(g->captured_by_black, g->cap_black_count, COLOR_WHITE, out, size);
      break;
    case 8:
      snprintf(out, size, " %s────────────────────────────────────%s", COLOR_GRID, COLOR_RESET);
      break;
    case 9:
      snprintf(out, size, " %sRECENT MOVES (SAN)%s", COLOR_GOLD, COLOR_RESET);
      break;
    case 10:
    case 11:
    case 12:
    case 13:
    case 14: {
      int total_full_turns = (g->history_count + 1) / 2;
      int start_turn = (total_full_turns > 5) ? (total_full_turns - 5) : 0;
      int turn_idx = start_turn + (row_idx - 10);
      int w_idx = turn_idx * 2;
      int b_idx = w_idx + 1;
      if (w_idx < g->history_count) {
        const char *w_san = g->history[w_idx].san;
        const char *b_san = (b_idx < g->history_count) ? g->history[b_idx].san : "";
        snprintf(out, size, "  %s%2d.%s %-10s %-10s",
                 COLOR_DIM, turn_idx + 1, COLOR_RESET, w_san, b_san);
      }
      break;
    }
    case 15:
      snprintf(out, size, " %s────────────────────────────────────%s", COLOR_GRID, COLOR_RESET);
      break;
    case 16:
      snprintf(out, size, " %s[WASD/Arrows]%s Cursor  %s[Space]%s Select",
               COLOR_CYAN, COLOR_RESET, COLOR_GOLD, COLOR_RESET);
      break;
    case 17:
      snprintf(out, size, " %s[Mouse Click]%s Select / Move Square",
               COLOR_CYAN, COLOR_RESET);
      break;
    case 18:
      snprintf(out, size, " %s[M]%s Mode  %s[F]%s Flip  %s[U]%s Undo  %s[H]%s Hint",
               COLOR_GOLD, COLOR_RESET, COLOR_CYAN, COLOR_RESET,
               COLOR_GOLD, COLOR_RESET, COLOR_CYAN, COLOR_RESET);
      break;
    default:
      break;
  }
}

void render_game(const GameState *g) {
  printf("\033[H");

  /* Line 1: Top outer border */
  printf("  %s╔" TABLE_BORDER_HORIZ "╗%s\033[K\n", COLOR_BORDER, COLOR_RESET);

  /* Line 2: Header bar */
  char header[512];
  snprintf(header, sizeof(header),
           " %s♔ ♕%s %sCHESS MASTER CLI%s %s♜ ♞%s │ %sMODE: %-10s%s │ %sWINS: %-3d%s │ %sCURSOR: %c%d%s",
           FG_WHITE_PIECE, COLOR_RESET,
           COLOR_TITLE, COLOR_RESET,
           FG_BLACK_PIECE, COLOR_RESET,
           COLOR_CYAN, game_difficulty_name(g->difficulty), COLOR_RESET,
           COLOR_GOLD, g->wins_player, COLOR_RESET,
           COLOR_GOLD, (char)('a' + g->cursor_c), 8 - g->cursor_r, COLOR_RESET);
  print_table_line(header);

  /* Line 3: Top separator */
  printf("  %s╠" TABLE_BORDER_HORIZ "╣%s\033[K\n", COLOR_BORDER, COLOR_RESET);

  /* Lines 4..22 (19 rows): Board (Left 55 cols) + Divider + Sidebar (Right 38 cols) */
  const char *files_label = g->board_flipped
    ? "      h     g     f     e     d     c     b     a      "
    : "      a     b     c     d     e     f     g     h      ";

  for (int b_row = 0; b_row < 19; b_row++) {
    char left_buf[1024];
    char right_buf[512];

    if (b_row == 0 || b_row == 18) {
      snprintf(left_buf, sizeof(left_buf), "%s%s%s", COLOR_DIM, files_label, COLOR_RESET);
    } else if (b_row == 1) {
      snprintf(left_buf, sizeof(left_buf),
               "   %s┌─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┐%s   ",
               COLOR_GRID, COLOR_RESET);
    } else if (b_row == 17) {
      snprintf(left_buf, sizeof(left_buf),
               "   %s└─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┘%s   ",
               COLOR_GRID, COLOR_RESET);
    } else if (b_row % 2 == 1) {
      snprintf(left_buf, sizeof(left_buf),
               "   %s├─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┤%s   ",
               COLOR_GRID, COLOR_RESET);
    } else {
      int display_rank = (b_row - 2) / 2;
      int actual_r = g->board_flipped ? (7 - display_rank) : display_rank;
      int rank_num = 8 - actual_r;

      int pos = snprintf(left_buf, sizeof(left_buf), " %s%d%s %s│%s",
                         COLOR_DIM, rank_num, COLOR_RESET, COLOR_GRID, COLOR_RESET);
      for (int display_col = 0; display_col < 8; display_col++) {
        int actual_c = g->board_flipped ? (7 - display_col) : display_col;
        char cell_str[128];
        format_cell_5chars(g, actual_r, actual_c, cell_str, sizeof(cell_str));
        pos += snprintf(left_buf + pos, sizeof(left_buf) - pos, "%s%s│%s",
                        cell_str, COLOR_GRID, COLOR_RESET);
      }
      snprintf(left_buf + pos, sizeof(left_buf) - pos, " %s%d%s ",
               COLOR_DIM, rank_num, COLOR_RESET);
    }

    format_sidebar_row(g, b_row, right_buf, sizeof(right_buf));
    print_split_line(left_buf, right_buf);
  }

  /* Line 23: Bottom separator */
  printf("  %s╠" TABLE_BORDER_HORIZ "╣%s\033[K\n", COLOR_BORDER, COLOR_RESET);

  /* Line 24: Status & Controls bar */
  char footer[512];
  char msg_trunc[56];
  snprintf(msg_trunc, sizeof(msg_trunc), "%.52s", g->status_msg);
  snprintf(footer, sizeof(footer),
           " %s[N]%sNew %s[U]%sUndo %s[H]%sHint %s[M]%sAI %s[F]%sFlip %s[Q]%sQuit │ %s%s%s",
           COLOR_GOLD, COLOR_RESET,
           COLOR_CYAN, COLOR_RESET,
           COLOR_GOLD, COLOR_RESET,
           COLOR_CYAN, COLOR_RESET,
           COLOR_GOLD, COLOR_RESET,
           COLOR_CYAN, COLOR_RESET,
           g->awaiting_promotion ? COLOR_GOLD : COLOR_TITLE,
           msg_trunc,
           COLOR_RESET);
  print_table_line(footer);

  /* Line 25: Bottom outer border */
  printf("  %s╚" TABLE_BORDER_HORIZ "╝%s\033[K\n", COLOR_BORDER, COLOR_RESET);

  fflush(stdout);
}
