#include "render.h"
#include <stdio.h>
#include <string.h>

#define COLOR_RESET        "\033[0m"
#define COLOR_BORDER       "\033[38;2;36;140;80m"       /* Casino Emerald Green */
#define COLOR_BORDER_DIM   "\033[38;2;24;90;55m"
#define COLOR_TITLE        "\033[1;38;2;255;255;255m"
#define COLOR_GOLD         "\033[1;38;2;255;215;60m"
#define COLOR_CYAN         "\033[1;38;2;0;230;225m"
#define COLOR_DIM          "\033[38;2;130;150;170m"
#define COLOR_CARD_BORDER  "\033[38;2;170;190;210m"
#define COLOR_CARD_SEL     "\033[1;38;2;255;230;80;48;2;35;65;105m"
#define COLOR_CURSOR       "\033[1;38;2;0;255;220;48;2;25;50;80m"
#define COLOR_RED_SUIT     "\033[1;38;2;255;70;70m"
#define COLOR_BLACK_SUIT   "\033[1;38;2;235;245;255m"
#define COLOR_HIDDEN_CARD  "\033[38;2;100;140;215m"
#define COLOR_WIN          "\033[1;38;2;75;255;120m"

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
      if (s[i] == 'm') {
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
  int pad = 75 - vis;
  if (pad < 0) pad = 0;
  printf("  %s║%s%s%*s%s║%s\033[K\n",
         COLOR_BORDER, COLOR_RESET, content, pad, "", COLOR_BORDER, COLOR_RESET);
}

static const char *get_suit_color(Suit s) {
  if (s == SUIT_HEARTS || s == SUIT_DIAMONDS) {
    return COLOR_RED_SUIT;
  }
  return COLOR_BLACK_SUIT;
}

// Format a single 7-char card face row: "│ 10♠ │" or "│ A ♥ │"
static void format_card_face(Card card, bool is_selected, bool is_cursor, char *out, size_t size) {
  const char *r_str = rank_name(card.rank);
  const char *s_str = suit_symbol(card.suit);
  const char *scol = get_suit_color(card.suit);
  const char *bcol = is_selected ? COLOR_CARD_SEL : (is_cursor ? COLOR_CURSOR : COLOR_CARD_BORDER);

  if (card.rank == RANK_10) {
    snprintf(out, size, "%s│%s10%s %s│%s", bcol, scol, s_str, bcol, COLOR_RESET);
  } else {
    snprintf(out, size, "%s│%s %s %s %s│%s", bcol, scol, r_str, s_str, bcol, COLOR_RESET);
  }
}

// Format top card box in 3 rows: row 0 = top, row 1 = face, row 2 = bot
static void format_card_full_row(Card card, bool face_down, int row, bool is_selected, bool is_cursor, char *out, size_t size) {
  const char *bcol = is_selected ? COLOR_CARD_SEL : (is_cursor ? COLOR_CURSOR : COLOR_CARD_BORDER);

  if (face_down) {
    if (row == 0) snprintf(out, size, "%s┌─────┐%s", COLOR_HIDDEN_CARD, COLOR_RESET);
    else if (row == 1) snprintf(out, size, "%s│░░░░░│%s", COLOR_HIDDEN_CARD, COLOR_RESET);
    else if (row == 2) snprintf(out, size, "%s└─────┘%s", COLOR_HIDDEN_CARD, COLOR_RESET);
    else out[0] = '\0';
    return;
  }

  if (row == 0) {
    snprintf(out, size, "%s┌─────┐%s", bcol, COLOR_RESET);
  } else if (row == 1) {
    format_card_face(card, is_selected, is_cursor, out, size);
  } else if (row == 2) {
    snprintf(out, size, "%s└─────┘%s", bcol, COLOR_RESET);
  } else {
    out[0] = '\0';
  }
}

PileType render_get_pile_at_pos(int mouse_x, int mouse_y, int *out_card_idx) {
  if (out_card_idx) *out_card_idx = -1;

  // Inner box starts at x = 4 (leading 2 spaces + ║ + 1 space = col 4)
  // Check Top Row cards (y = 4, 5, 6)
  if (mouse_y >= 4 && mouse_y <= 6) {
    // Stock: x 5..11
    if (mouse_x >= 5 && mouse_x <= 11) return PILE_STOCK;
    // Waste: x 15..21
    if (mouse_x >= 15 && mouse_x <= 21) return PILE_WASTE;
    // Foundations: F1 (x 35..41), F2 (45..51), F3 (55..61), F4 (65..71)
    if (mouse_x >= 35 && mouse_x <= 41) return PILE_FOUNDATION_1;
    if (mouse_x >= 45 && mouse_x <= 51) return PILE_FOUNDATION_2;
    if (mouse_x >= 55 && mouse_x <= 61) return PILE_FOUNDATION_3;
    if (mouse_x >= 65 && mouse_x <= 71) return PILE_FOUNDATION_4;
  }

  // Check Tableau cards (y = 9..18)
  if (mouse_y >= 9 && mouse_y <= 18) {
    int col_idx = -1;
    if (mouse_x >= 5 && mouse_x <= 11) col_idx = 0;
    else if (mouse_x >= 15 && mouse_x <= 21) col_idx = 1;
    else if (mouse_x >= 25 && mouse_x <= 31) col_idx = 2;
    else if (mouse_x >= 35 && mouse_x <= 41) col_idx = 3;
    else if (mouse_x >= 45 && mouse_x <= 51) col_idx = 4;
    else if (mouse_x >= 55 && mouse_x <= 61) col_idx = 5;
    else if (mouse_x >= 65 && mouse_x <= 71) col_idx = 6;

    if (col_idx != -1) {
      if (out_card_idx) {
        *out_card_idx = mouse_y - 9;
      }
      return (PileType)(PILE_TABLEAU_1 + col_idx);
    }
  }

  return PILE_NONE;
}

void render_game(const GameState *game) {
  // Move cursor to top-left
  printf("\033[H");

  // Line 1: Top border (75 double horizontals between corners)
  printf("  %s╔═══════════════════════════════════════════════════════════════════════════╗%s\033[K\n",
         COLOR_BORDER, COLOR_RESET);

  // Line 2: Header / Stats bar
  char header_content[512];
  int mins = game->time_elapsed_sec / 60;
  int secs = game->time_elapsed_sec % 60;
  snprintf(header_content, sizeof(header_content),
           " %s♠ ♥%s %sKLONDIKE SOLITAIRE%s %s♦ ♣%s │ %sSCORE: %-4d%s │ %sMOVES: %-3d%s │ %sTIME: %02d:%02d%s │ %sBEST: %-4d%s",
           COLOR_RED_SUIT, COLOR_RESET,
           COLOR_TITLE, COLOR_RESET,
           COLOR_RED_SUIT, COLOR_RESET,
           COLOR_GOLD, game->score, COLOR_RESET,
           COLOR_CYAN, game->moves, COLOR_RESET,
           COLOR_DIM, mins, secs, COLOR_RESET,
           COLOR_GOLD, game->high_score, COLOR_RESET);
  print_table_line(header_content);

  // Line 3: Separator
  printf("  %s╠═══════════════════════════════════════════════════════════════════════════╣%s\033[K\n",
         COLOR_BORDER, COLOR_RESET);

  // Line 4: Top row pile labels
  // Columns aligned at: Stock (offset 3), Waste (13), F1 (33), F2 (43), F3 (53), F4 (63)
  char top_labels[512];
  snprintf(top_labels, sizeof(top_labels),
           "   %s[S] STOCK%s   %s[W] WASTE%s             %s[F1] ♠%s     %s[F2] ♥%s     %s[F3] ♦%s     %s[F4] ♣%s",
           (game->cursor_pile == PILE_STOCK) ? COLOR_CURSOR : COLOR_DIM, COLOR_RESET,
           (game->cursor_pile == PILE_WASTE) ? COLOR_CURSOR : COLOR_DIM, COLOR_RESET,
           (game->cursor_pile == PILE_FOUNDATION_1) ? COLOR_CURSOR : COLOR_DIM, COLOR_RESET,
           (game->cursor_pile == PILE_FOUNDATION_2) ? COLOR_CURSOR : COLOR_DIM, COLOR_RESET,
           (game->cursor_pile == PILE_FOUNDATION_3) ? COLOR_CURSOR : COLOR_DIM, COLOR_RESET,
           (game->cursor_pile == PILE_FOUNDATION_4) ? COLOR_CURSOR : COLOR_DIM, COLOR_RESET);
  print_table_line(top_labels);

  // Lines 5..7: Top row cards (Stock, Waste, Foundation 1-4) in 3 rows
  for (int row = 0; row < 3; row++) {
    char stock_str[128];
    char waste_str[128];
    char f_str[4][128];

    // Stock
    bool stock_sel = (game->selected_pile == PILE_STOCK);
    bool stock_cur = (game->cursor_pile == PILE_STOCK);
    const char *stk_col = stock_sel ? COLOR_CARD_SEL : (stock_cur ? COLOR_CURSOR : COLOR_HIDDEN_CARD);
    if (game->stock.count > 0) {
      if (row == 0) snprintf(stock_str, sizeof(stock_str), "%s┌─────┐%s", stk_col, COLOR_RESET);
      else if (row == 1) snprintf(stock_str, sizeof(stock_str), "%s│░ %2d░│%s", stk_col, game->stock.count, COLOR_RESET);
      else snprintf(stock_str, sizeof(stock_str), "%s└─────┘%s", stk_col, COLOR_RESET);
    } else {
      if (row == 0) snprintf(stock_str, sizeof(stock_str), "%s┌ ··· ┐%s", stk_col, COLOR_RESET);
      else if (row == 1) snprintf(stock_str, sizeof(stock_str), "%s│  O  │%s", stk_col, COLOR_RESET);
      else snprintf(stock_str, sizeof(stock_str), "%s└ ··· ┘%s", stk_col, COLOR_RESET);
    }

    // Waste
    bool waste_sel = (game->selected_pile == PILE_WASTE);
    bool waste_cur = (game->cursor_pile == PILE_WASTE);
    if (game->waste.count > 0) {
      Card top = game->waste.cards[game->waste.count - 1];
      format_card_full_row(top, false, row, waste_sel, waste_cur, waste_str, sizeof(waste_str));
    } else {
      if (row == 0) snprintf(waste_str, sizeof(waste_str), "%s┌ ··· ┐%s", COLOR_BORDER_DIM, COLOR_RESET);
      else if (row == 1) snprintf(waste_str, sizeof(waste_str), "%s│     │%s", COLOR_BORDER_DIM, COLOR_RESET);
      else snprintf(waste_str, sizeof(waste_str), "%s└ ··· ┘%s", COLOR_BORDER_DIM, COLOR_RESET);
    }

    // Foundations 0..3
    for (int f = 0; f < NUM_FOUNDATIONS; f++) {
      PileType fp = (PileType)(PILE_FOUNDATION_1 + f);
      bool f_sel = (game->selected_pile == fp);
      bool f_cur = (game->cursor_pile == fp);
      if (game->foundations[f].count > 0) {
        Card top = game->foundations[f].cards[game->foundations[f].count - 1];
        format_card_full_row(top, false, row, f_sel, f_cur, f_str[f], sizeof(f_str[f]));
      } else {
        static const char *f_suits[] = { "♠", "♥", "♦", "♣" };
        const char *scol = (f == 1 || f == 2) ? COLOR_RED_SUIT : COLOR_BLACK_SUIT;
        const char *fbcol = f_sel ? COLOR_CARD_SEL : (f_cur ? COLOR_CURSOR : COLOR_BORDER_DIM);
        if (row == 0) snprintf(f_str[f], sizeof(f_str[f]), "%s┌ ··· ┐%s", fbcol, COLOR_RESET);
        else if (row == 1) snprintf(f_str[f], sizeof(f_str[f]), "%s│  %s%s%s  │%s", fbcol, scol, f_suits[f], fbcol, COLOR_RESET);
        else snprintf(f_str[f], sizeof(f_str[f]), "%s└ ··· ┘%s", fbcol, COLOR_RESET);
      }
    }

    char line_buf[1024];
    snprintf(line_buf, sizeof(line_buf),
             "   %s   %s             %s   %s   %s   %s",
             stock_str, waste_str, f_str[0], f_str[1], f_str[2], f_str[3]);
    print_table_line(line_buf);
  }

  // Line 8: Separator
  printf("  %s╠═══════════════════════════════════════════════════════════════════════════╣%s\033[K\n",
         COLOR_BORDER, COLOR_RESET);

  // Line 9: Tableau column headers [1] .. [7]
  char tableau_headers[512];
  snprintf(tableau_headers, sizeof(tableau_headers),
           "   %s  [1]  %s   %s  [2]  %s   %s  [3]  %s   %s  [4]  %s   %s  [5]  %s   %s  [6]  %s   %s  [7]  %s",
           (game->cursor_pile == PILE_TABLEAU_1) ? COLOR_CURSOR : COLOR_CYAN, COLOR_RESET,
           (game->cursor_pile == PILE_TABLEAU_2) ? COLOR_CURSOR : COLOR_CYAN, COLOR_RESET,
           (game->cursor_pile == PILE_TABLEAU_3) ? COLOR_CURSOR : COLOR_CYAN, COLOR_RESET,
           (game->cursor_pile == PILE_TABLEAU_4) ? COLOR_CURSOR : COLOR_CYAN, COLOR_RESET,
           (game->cursor_pile == PILE_TABLEAU_5) ? COLOR_CURSOR : COLOR_CYAN, COLOR_RESET,
           (game->cursor_pile == PILE_TABLEAU_6) ? COLOR_CURSOR : COLOR_CYAN, COLOR_RESET,
           (game->cursor_pile == PILE_TABLEAU_7) ? COLOR_CURSOR : COLOR_CYAN, COLOR_RESET);
  print_table_line(tableau_headers);

  // Lines 10..19: Tableau cards (10 lines total)
  // For each column, prepare what to render on each of the 10 rows
  char col_rows[NUM_TABLEAU][10][128];
  for (int c = 0; c < NUM_TABLEAU; c++) {
    for (int r = 0; r < 10; r++) {
      col_rows[c][r][0] = '\0';
    }

    const CardPile *pile = &game->tableau[c];
    PileType pt = (PileType)(PILE_TABLEAU_1 + c);
    bool is_col_sel = (game->selected_pile == pt);
    bool is_col_cur = (game->cursor_pile == pt);

    if (pile->count == 0) {
      snprintf(col_rows[c][0], sizeof(col_rows[c][0]), "%s┌ ··· ┐%s", COLOR_BORDER_DIM, COLOR_RESET);
      snprintf(col_rows[c][1], sizeof(col_rows[c][1]), "%s│  K  │%s", COLOR_BORDER_DIM, COLOR_RESET);
      snprintf(col_rows[c][2], sizeof(col_rows[c][2]), "%s└ ··· ┘%s", COLOR_BORDER_DIM, COLOR_RESET);
      for (int r = 3; r < 10; r++) {
        snprintf(col_rows[c][r], sizeof(col_rows[c][r]), "       ");
      }
      continue;
    }

    // Determine how many face-down vs face-up cards
    int hidden_count = 0;
    while (hidden_count < pile->count && !pile->cards[hidden_count].face_up) {
      hidden_count++;
    }
    int up_count = pile->count - hidden_count;

    // Layout rows:
    // If total rows needed (hidden + up) <= 9, render 1 line per hidden card
    // Otherwise compress hidden cards into 1 line: "│░(%d)░│"
    int r_idx = 0;
    if (hidden_count > 0) {
      if (hidden_count + up_count <= 9) {
        for (int h = 0; h < hidden_count && r_idx < 9; h++) {
          snprintf(col_rows[c][r_idx++], sizeof(col_rows[c][0]), "%s│░░░░░│%s", COLOR_HIDDEN_CARD, COLOR_RESET);
        }
      } else {
        // Compressed hidden cards
        snprintf(col_rows[c][r_idx++], sizeof(col_rows[c][0]), "%s│░(%d)░│%s", COLOR_HIDDEN_CARD, hidden_count, COLOR_RESET);
      }
    }

    // Now render face-up cards
    // If up_count cards don't fit in remaining (10 - r_idx) lines, scroll down so bottom is shown
    int avail_face_rows = 9 - r_idx;
    int start_up_idx = 0;
    if (up_count > avail_face_rows) {
      start_up_idx = up_count - avail_face_rows;
      snprintf(col_rows[c][r_idx++], sizeof(col_rows[c][0]), "%s▲ +%-2d │%s", COLOR_DIM, start_up_idx, COLOR_RESET);
    }

    for (int u = start_up_idx; u < up_count && r_idx < 9; u++) {
      int card_actual_idx = hidden_count + u;
      Card cd = pile->cards[card_actual_idx];

      bool sel = is_col_sel && (card_actual_idx >= game->selected_card_idx);
      bool cur = is_col_cur && (card_actual_idx == game->cursor_card_idx);

      format_card_face(cd, sel, cur, col_rows[c][r_idx++], sizeof(col_rows[c][0]));
    }

    // Bottom border for the stack
    if (r_idx < 10) {
      bool sel = is_col_sel;
      const char *bcol = sel ? COLOR_CARD_SEL : COLOR_CARD_BORDER;
      snprintf(col_rows[c][r_idx++], sizeof(col_rows[c][0]), "%s└─────┘%s", bcol, COLOR_RESET);
    }

    // Fill remaining with blanks
    while (r_idx < 10) {
      snprintf(col_rows[c][r_idx++], sizeof(col_rows[c][0]), "       ");
    }
  }

  // Print the 10 rows of the tableau
  for (int r = 0; r < 10; r++) {
    char tableau_line[1024];
    snprintf(tableau_line, sizeof(tableau_line),
             "   %.63s   %.63s   %.63s   %.63s   %.63s   %.63s   %.63s",
             col_rows[0][r], col_rows[1][r], col_rows[2][r],
             col_rows[3][r], col_rows[4][r], col_rows[5][r], col_rows[6][r]);
    print_table_line(tableau_line);
  }

  // Line 20: Controls & Status message
  char status_line[512];
  if (game->status == STATE_WON) {
    snprintf(status_line, sizeof(status_line),
             " %s★ VICTORY! %s ★  [N] New Game  [R] Replay  [Q] Exit",
             COLOR_WIN, game->status_msg);
  } else {
    snprintf(status_line, sizeof(status_line),
             " %s[S]%sDraw %s[W]%sWaste %s[1-7]%sCol %s[F]%sFound %s[A]%sAuto %s[U]%sUndo %s[H]%sHint │ %s",
             COLOR_GOLD, COLOR_RESET,
             COLOR_GOLD, COLOR_RESET,
             COLOR_CYAN, COLOR_RESET,
             COLOR_GOLD, COLOR_RESET,
             COLOR_CYAN, COLOR_RESET,
             COLOR_GOLD, COLOR_RESET,
             COLOR_CYAN, COLOR_RESET,
             game->status_msg);
  }
  print_table_line(status_line);

  // Line 21: Bottom border
  printf("  %s╚═══════════════════════════════════════════════════════════════════════════╝%s\033[K\n",
         COLOR_BORDER, COLOR_RESET);

  fflush(stdout);
}
