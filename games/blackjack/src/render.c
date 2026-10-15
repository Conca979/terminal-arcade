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
#define COLOR_CARD_BORDER  "\033[38;2;160;180;200m"
#define COLOR_RED_SUIT     "\033[1;38;2;255;70;70m"
#define COLOR_BLACK_SUIT   "\033[1;38;2;230;242;255m"
#define COLOR_HOLE_CARD    "\033[1;38;2;110;145;215m"

#define COLOR_WIN          "\033[1;38;2;75;255;120m"
#define COLOR_LOSE         "\033[1;38;2;255;85;85m"
#define COLOR_PUSH         "\033[1;38;2;255;215;60m"
#define COLOR_BJ           "\033[1;38;2;0;245;255m"

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
      // UTF-8 continuation bytes (0x80 to 0xBF) do not take additional visual columns
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

static const char *get_rank_str(Rank r) {
  switch (r) {
    case RANK_2: return "2";
    case RANK_3: return "3";
    case RANK_4: return "4";
    case RANK_5: return "5";
    case RANK_6: return "6";
    case RANK_7: return "7";
    case RANK_8: return "8";
    case RANK_9: return "9";
    case RANK_10: return "10";
    case RANK_J: return "J";
    case RANK_Q: return "Q";
    case RANK_K: return "K";
    case RANK_A: return "A";
    default: return "?";
  }
}

static const char *get_suit_str(Suit s) {
  switch (s) {
    case SUIT_SPADES:   return "♠";
    case SUIT_HEARTS:   return "♥";
    case SUIT_DIAMONDS: return "♦";
    case SUIT_CLUBS:    return "♣";
    default: return "?";
  }
}

static const char *get_suit_color(Suit s) {
  if (s == SUIT_HEARTS || s == SUIT_DIAMONDS) {
    return COLOR_RED_SUIT;
  }
  return COLOR_BLACK_SUIT;
}

// Format a single card line (row 0..4) into out_buf (exact visible width: 7 columns)
static void get_card_line(Card card, bool face_down, int row, char *out_buf, size_t buf_size) {
  if (face_down) {
    switch (row) {
      case 0:
        snprintf(out_buf, buf_size, "%s┌─────┐%s", COLOR_HOLE_CARD, COLOR_RESET);
        break;
      case 1:
        snprintf(out_buf, buf_size, "%s│░░░░░│%s", COLOR_HOLE_CARD, COLOR_RESET);
        break;
      case 2:
        snprintf(out_buf, buf_size, "%s│░ ? ░│%s", COLOR_HOLE_CARD, COLOR_RESET);
        break;
      case 3:
        snprintf(out_buf, buf_size, "%s│░░░░░│%s", COLOR_HOLE_CARD, COLOR_RESET);
        break;
      case 4:
        snprintf(out_buf, buf_size, "%s└─────┘%s", COLOR_HOLE_CARD, COLOR_RESET);
        break;
      default:
        out_buf[0] = '\0';
        break;
    }
    return;
  }

  const char *r_str = get_rank_str(card.rank);
  const char *s_str = get_suit_str(card.suit);
  const char *scol = get_suit_color(card.suit);

  switch (row) {
    case 0:
      snprintf(out_buf, buf_size, "%s┌─────┐%s", COLOR_CARD_BORDER, COLOR_RESET);
      break;
    case 1:
      if (card.rank == RANK_10) {
        snprintf(out_buf, buf_size, "%s│%s10  %s%s│%s",
                 COLOR_CARD_BORDER, scol, s_str, COLOR_CARD_BORDER, COLOR_RESET);
      } else {
        snprintf(out_buf, buf_size, "%s│%s%s   %s%s│%s",
                 COLOR_CARD_BORDER, scol, r_str, s_str, COLOR_CARD_BORDER, COLOR_RESET);
      }
      break;
    case 2:
      snprintf(out_buf, buf_size, "%s│  %s%s  %s│%s",
               COLOR_CARD_BORDER, scol, s_str, COLOR_CARD_BORDER, COLOR_RESET);
      break;
    case 3:
      if (card.rank == RANK_10) {
        snprintf(out_buf, buf_size, "%s│%s%s  10%s│%s",
                 COLOR_CARD_BORDER, scol, s_str, COLOR_CARD_BORDER, COLOR_RESET);
      } else {
        snprintf(out_buf, buf_size, "%s│%s%s   %s%s│%s",
                 COLOR_CARD_BORDER, scol, s_str, r_str, COLOR_CARD_BORDER, COLOR_RESET);
      }
      break;
    case 4:
      snprintf(out_buf, buf_size, "%s└─────┘%s", COLOR_CARD_BORDER, COLOR_RESET);
      break;
    default:
      out_buf[0] = '\0';
      break;
  }
}

void render_game(const GameState *game) {
  // Move cursor to top-left
  printf("\033[H");

  // Line 1: Top border (75 double horizontals between corners)
  printf("  %s╔═══════════════════════════════════════════════════════════════════════════╗%s\033[K\n",
         COLOR_BORDER, COLOR_RESET);

  // Line 2: Header / Bankroll bar
  int shoe_pct = ((SHOE_SIZE - game->shoe_index) * 100) / SHOE_SIZE;
  char header_content[256];
  snprintf(header_content, sizeof(header_content),
           " %s♠ ♥%s %sBLACKJACK 21%s %s♦ ♣%s  │  %sCHIPS: $%d%s  │  %sHIGH: $%d%s  │  %sSHOE: %d%%%s",
           COLOR_RED_SUIT, COLOR_RESET,
           COLOR_TITLE, COLOR_RESET,
           COLOR_RED_SUIT, COLOR_RESET,
           COLOR_GOLD, game->chips, COLOR_RESET,
           COLOR_CYAN, game->high_chips, COLOR_RESET,
           COLOR_DIM, shoe_pct, COLOR_RESET);
  print_table_line(header_content);

  // Line 3: Separator
  printf("  %s╠═══════════════════════════════════════════════════════════════════════════╣%s\033[K\n",
         COLOR_BORDER, COLOR_RESET);

  // Line 4: Dealer header
  char dealer_info[128];
  if (game->state == STATE_BETTING) {
    snprintf(dealer_info, sizeof(dealer_info), "  DEALER'S HAND");
  } else if (!game->dealer_hole_revealed) {
    snprintf(dealer_info, sizeof(dealer_info), "  DEALER'S HAND   [Showing: %d + ?]",
             card_value(game->dealer_hand.cards[0]));
  } else {
    bool is_soft = false;
    int d_tot = hand_calculate_total(&game->dealer_hand, &is_soft);
    if (d_tot > 21) {
      snprintf(dealer_info, sizeof(dealer_info), "  DEALER'S HAND   [Total: %d - BUSTED!]", d_tot);
    } else if (game->dealer_hand.count == 2 && d_tot == 21) {
      snprintf(dealer_info, sizeof(dealer_info), "  DEALER'S HAND   [BLACKJACK!]");
    } else {
      snprintf(dealer_info, sizeof(dealer_info), "  DEALER'S HAND   [Total: %d%s]",
               d_tot, is_soft ? " (Soft)" : "");
    }
  }
  print_table_line(dealer_info);

  // Lines 5-9: Dealer cards (or Casino intro in betting phase)
  if (game->state == STATE_BETTING) {
    char b1[256], b2[256], b3[256], b4[256];
    snprintf(b1, sizeof(b1), "         %s[ CASINO DEALER ]%s  Shoe: 6 Decks (312 Cards) • 75%% Penetration", COLOR_GOLD, COLOR_RESET);
    snprintf(b2, sizeof(b2), "         Dealer stands on 17+ • Natural Blackjack pays 3:2 • Insurance 2:1");
    snprintf(b3, sizeof(b3), "         Double Down on any initial 2 cards • Split pairs for dual hands");
    snprintf(b4, sizeof(b4), "         Late Surrender available on initial 2 cards (reclaims 50%% bet)");

    print_table_line(b1);
    print_table_line(b2);
    print_table_line(b3);
    print_table_line(b4);
    print_table_line("");
  } else {
    for (int r = 0; r < 5; r++) {
      char row_buf[512] = "    ";
      for (int i = 0; i < game->dealer_hand.count; i++) {
        bool face_down = (i == 1 && !game->dealer_hole_revealed);
        char c_buf[128];
        get_card_line(game->dealer_hand.cards[i], face_down, r, c_buf, sizeof(c_buf));
        strcat(row_buf, c_buf);
        if (i + 1 < game->dealer_hand.count) {
          strcat(row_buf, " ");
        }
      }
      print_table_line(row_buf);
    }
  }

  // Line 10: Divider
  printf("  %s╟───────────────────────────────────────────────────────────────────────────╢%s\033[K\n",
         COLOR_BORDER_DIM, COLOR_RESET);

  // Line 11: Player header
  char player_info[256];
  if (game->state == STATE_BETTING) {
    snprintf(player_info, sizeof(player_info), "  PLAYER'S HAND");
  } else if (game->player_hand_count == 1) {
    bool is_soft = false;
    int p_tot = hand_calculate_total(&game->player_hands[0], &is_soft);
    const char *status_tag = "";
    if (game->player_hands[0].is_blackjack) status_tag = " - BLACKJACK!";
    else if (game->player_hands[0].is_busted) status_tag = " - BUST!";
    else if (game->player_hands[0].is_standing) status_tag = " - STAND";

    snprintf(player_info, sizeof(player_info), "  PLAYER'S HAND   [Total: %d%s]  [Bet: $%d]%s",
             p_tot, is_soft ? " (Soft)" : "", game->player_hands[0].bet, status_tag);
  } else {
    // 2 Split hands
    int t0 = hand_calculate_total(&game->player_hands[0], NULL);
    int t1 = hand_calculate_total(&game->player_hands[1], NULL);
    snprintf(player_info, sizeof(player_info),
             "  HAND 1: %s[%d]%s [Bet: $%d]%s  │  HAND 2: %s[%d]%s [Bet: $%d]%s",
             (game->active_hand_idx == 0 && game->state == STATE_PLAYER_TURN) ? "▶ " : "",
             t0, game->player_hands[0].is_busted ? " (BUST)" : "",
             game->player_hands[0].bet,
             (game->active_hand_idx == 0 && game->state == STATE_PLAYER_TURN) ? " ◀" : "",
             (game->active_hand_idx == 1 && game->state == STATE_PLAYER_TURN) ? "▶ " : "",
             t1, game->player_hands[1].is_busted ? " (BUST)" : "",
             game->player_hands[1].bet,
             (game->active_hand_idx == 1 && game->state == STATE_PLAYER_TURN) ? " ◀" : "");
  }
  print_table_line(player_info);

  // Lines 12-16: Player cards (or Chip selector in betting phase)
  if (game->state == STATE_BETTING) {
    char p1[256], p2[256], p3[256], p4[256], p5[256];
    snprintf(p1, sizeof(p1), "                    %s[ PLACE YOUR BET FOR NEXT HAND ]%s", COLOR_CYAN, COLOR_RESET);
    snprintf(p2, sizeof(p2), "                            Current Bet: %s$%d%s", COLOR_GOLD, game->current_bet, COLOR_RESET);
    snprintf(p3, sizeof(p3), "          Presets: [1] $10   [2] $25   [3] $50   [4] $100   [5] $500");
    snprintf(p4, sizeof(p4), "          [↑/W] +$10   [↓/S] -$10   [A] All-In ($%d)", game->chips);
    snprintf(p5, sizeof(p5), "                  Press %s[Space]%s or %s[Enter]%s to DEAL CARDS",
             COLOR_GOLD, COLOR_RESET, COLOR_GOLD, COLOR_RESET);

    print_table_line(p1);
    print_table_line(p2);
    print_table_line(p3);
    print_table_line(p4);
    print_table_line(p5);
  } else if (game->player_hand_count == 1) {
    // Single hand cards
    for (int r = 0; r < 5; r++) {
      char row_buf[512] = "    ";
      for (int i = 0; i < game->player_hands[0].count; i++) {
        char c_buf[128];
        get_card_line(game->player_hands[0].cards[i], false, r, c_buf, sizeof(c_buf));
        strcat(row_buf, c_buf);
        if (i + 1 < game->player_hands[0].count) {
          strcat(row_buf, " ");
        }
      }
      print_table_line(row_buf);
    }
  } else {
    // 2 Split hands rendered side by side
    for (int r = 0; r < 5; r++) {
      char row_buf[512] = "  ";
      for (int i = 0; i < game->player_hands[0].count; i++) {
        char c_buf[128];
        get_card_line(game->player_hands[0].cards[i], false, r, c_buf, sizeof(c_buf));
        strcat(row_buf, c_buf);
        if (i + 1 < game->player_hands[0].count) {
          strcat(row_buf, " ");
        }
      }
      int vis0 = visible_length(row_buf);
      int pad0 = 36 - vis0;
      if (pad0 < 0) pad0 = 0;
      for (int s = 0; s < pad0; s++) {
        strcat(row_buf, " ");
      }

      char sep[64];
      snprintf(sep, sizeof(sep), "%s│%s  ", COLOR_BORDER_DIM, COLOR_RESET);
      strcat(row_buf, sep);

      for (int i = 0; i < game->player_hands[1].count; i++) {
        char c_buf[128];
        get_card_line(game->player_hands[1].cards[i], false, r, c_buf, sizeof(c_buf));
        strcat(row_buf, c_buf);
        if (i + 1 < game->player_hands[1].count) {
          strcat(row_buf, " ");
        }
      }

      print_table_line(row_buf);
    }
  }

  // Line 17: Result / Status banner
  const char *b_col = COLOR_CYAN;
  if (game->banner_color == 1) b_col = COLOR_WIN;
  else if (game->banner_color == 2) b_col = COLOR_LOSE;
  else if (game->banner_color == 3) b_col = COLOR_PUSH;
  else if (game->banner_color == 4) b_col = COLOR_BJ;

  const char *msg = (game->banner_msg[0] != '\0') ? game->banner_msg : game->status_msg;
  int msg_vis = visible_length(msg);
  int pad_total = 75 - msg_vis;
  if (pad_total < 0) pad_total = 0;
  int pad_left = pad_total / 2;

  char banner_content[256];
  snprintf(banner_content, sizeof(banner_content), "%*s%s%s%s",
           pad_left, "", b_col, msg, COLOR_RESET);
  print_table_line(banner_content);

  // Line 18: Bottom table border
  printf("  %s╚═══════════════════════════════════════════════════════════════════════════╝%s\033[K\n",
         COLOR_BORDER, COLOR_RESET);

  // Line 19: Action Controls bar
  if (game->state == STATE_BETTING) {
    printf("  %s[↑/↓/W/S]%s ±$10  %s[1-5]%s Preset  %s[A]%s All-In  %s[Space/Enter]%s Deal  %s[Q]%s Exit\033[K\n",
           COLOR_GOLD, COLOR_RESET, COLOR_GOLD, COLOR_RESET, COLOR_CYAN, COLOR_RESET, COLOR_GOLD, COLOR_RESET, COLOR_DIM, COLOR_RESET);
  } else if (game->state == STATE_INSURANCE) {
    printf("  %s[Y/I]%s Take Insurance ($%d, pays 2:1)  %s[N/Space/Enter]%s Decline  %s[Q]%s Exit\033[K\n",
           COLOR_GOLD, COLOR_RESET, game->current_bet / 2, COLOR_CYAN, COLOR_RESET, COLOR_DIM, COLOR_RESET);
  } else if (game->state == STATE_PLAYER_TURN) {
    const Hand *h = &game->player_hands[game->active_hand_idx];
    bool can_d = hand_can_double(h, game->chips);
    bool can_s = hand_can_split(h, game->chips);
    bool can_surr = hand_can_surrender(h, game->state);

    printf("  %s[H/Space]%s Hit  %s[S/Enter]%s Stand%s%s%s%s%s%s  %s[Q]%s Exit\033[K\n",
           COLOR_GOLD, COLOR_RESET, COLOR_CYAN, COLOR_RESET,
           can_d ? "  \033[1;38;2;255;215;60m[D]\033[0m Double" : "", "",
           can_s ? "  \033[1;38;2;255;215;60m[P]\033[0m Split" : "", "",
           can_surr ? "  \033[1;38;2;255;215;60m[U]\033[0m Surrender" : "", "",
           COLOR_DIM, COLOR_RESET);
  } else if (game->state == STATE_ROUND_OVER) {
    printf("  %s[Space/Enter]%s Next Hand  %s[R]%s Reload Bankroll  %s[Q/Esc]%s Exit to Arcade\033[K\n",
           COLOR_GOLD, COLOR_RESET, COLOR_CYAN, COLOR_RESET, COLOR_DIM, COLOR_RESET);
  } else {
    // GAME OVER
    printf("  %s[R/Space/Enter]%s Reload $%d Bankroll  %s[Q/Esc]%s Exit to Arcade\033[K\n",
           COLOR_GOLD, COLOR_RESET, RELOAD_AMOUNT, COLOR_DIM, COLOR_RESET);
  }

  // Line 20: Stats summary bar
  float win_pct = (game->hands_played > 0) ? ((float)game->hands_won * 100.0f / (float)game->hands_played) : 0.0f;
  printf("  %sHands: %d | Won: %d (%.1f%%) | High: $%d | Shoe: %d/%d cards | Min: $%d%s\033[K\n",
         COLOR_DIM,
         game->hands_played, game->hands_won, win_pct, game->high_chips,
         SHOE_SIZE - game->shoe_index, SHOE_SIZE, MIN_BET,
         COLOR_RESET);

  fflush(stdout);
}
