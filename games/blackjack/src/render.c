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

// Format a single card line (row 0..4) into out_buf (contains ANSI escape sequences)
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
        snprintf(out_buf, buf_size, "%s│%s10%s %s%s %s│%s",
                 COLOR_CARD_BORDER, scol, COLOR_RESET, scol, s_str, COLOR_CARD_BORDER, COLOR_RESET);
      } else {
        snprintf(out_buf, buf_size, "%s│%s%s  %s %s %s│%s",
                 COLOR_CARD_BORDER, scol, r_str, scol, s_str, COLOR_CARD_BORDER, COLOR_RESET);
      }
      break;
    case 2:
      snprintf(out_buf, buf_size, "%s│  %s%s  %s│%s",
               COLOR_CARD_BORDER, scol, s_str, COLOR_CARD_BORDER, COLOR_RESET);
      break;
    case 3:
      if (card.rank == RANK_10) {
        snprintf(out_buf, buf_size, "%s│ %s%s %s10%s│%s",
                 COLOR_CARD_BORDER, scol, s_str, scol, COLOR_CARD_BORDER, COLOR_RESET);
      } else {
        snprintf(out_buf, buf_size, "%s│ %s%s  %s%s%s│%s",
                 COLOR_CARD_BORDER, scol, s_str, scol, r_str, COLOR_CARD_BORDER, COLOR_RESET);
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
  printf("  %s╔═══════════════════════════════════════════════════════════════════════════╗%s\n",
         COLOR_BORDER, COLOR_RESET);

  // Line 2: Header / Bankroll bar
  // Inner width: 75 chars
  int shoe_pct = ((SHOE_SIZE - game->shoe_index) * 100) / SHOE_SIZE;
  char header_bar[256];
  snprintf(header_bar, sizeof(header_bar),
           " %s♠ ♥%s %sBLACKJACK 21%s %s♦ ♣%s  |  %sCHIPS: $%d%s  |  %sHIGH: $%d%s  |  %sSHOE: %d%%%s",
           COLOR_RED_SUIT, COLOR_RESET,
           COLOR_TITLE, COLOR_RESET,
           COLOR_RED_SUIT, COLOR_RESET,
           COLOR_GOLD, game->chips, COLOR_RESET,
           COLOR_CYAN, game->high_chips, COLOR_RESET,
           COLOR_DIM, shoe_pct, COLOR_RESET);

  // Visible length is 73 characters, so pad with 2 spaces
  printf("  %s║%s %s%*s %s║%s\n",
         COLOR_BORDER, COLOR_RESET,
         header_bar,
         (int)(73 - (26 + 10 + ((game->chips > 999) ? 1 : 0) + 11 + ((game->high_chips > 999) ? 1 : 0) + 11)), "",
         COLOR_BORDER, COLOR_RESET);

  // Line 3: Separator
  printf("  %s╠═══════════════════════════════════════════════════════════════════════════╣%s\n",
         COLOR_BORDER, COLOR_RESET);

  // Line 4: Dealer header
  char dealer_info[128];
  if (game->state == STATE_BETTING) {
    snprintf(dealer_info, sizeof(dealer_info), "DEALER'S HAND");
  } else if (!game->dealer_hole_revealed) {
    snprintf(dealer_info, sizeof(dealer_info), "DEALER'S HAND   [Showing: %d + ?]",
             card_value(game->dealer_hand.cards[0]));
  } else {
    bool is_soft = false;
    int d_tot = hand_calculate_total(&game->dealer_hand, &is_soft);
    if (d_tot > 21) {
      snprintf(dealer_info, sizeof(dealer_info), "DEALER'S HAND   [Total: %d - BUSTED!]", d_tot);
    } else if (game->dealer_hand.count == 2 && d_tot == 21) {
      snprintf(dealer_info, sizeof(dealer_info), "DEALER'S HAND   [BLACKJACK!]");
    } else {
      snprintf(dealer_info, sizeof(dealer_info), "DEALER'S HAND   [Total: %d%s]",
               d_tot, is_soft ? " (Soft)" : "");
    }
  }
  printf("  %s║%s  %-71s  %s║%s\n", COLOR_BORDER, COLOR_RESET, dealer_info, COLOR_BORDER, COLOR_RESET);

  // Lines 5-9: Dealer cards (or Casino intro in betting phase)
  if (game->state == STATE_BETTING) {
    printf("  %s║%s         %s[ CASINO DEALER ]%s  Shoe: 6 Decks (312 Cards) • 75%% Penetration        %s║%s\n",
           COLOR_BORDER, COLOR_RESET, COLOR_GOLD, COLOR_RESET, COLOR_BORDER, COLOR_RESET);
    printf("  %s║%s         Dealer stands on 17+ • Natural Blackjack pays 3:2 • Insurance 2:1   %s║%s\n",
           COLOR_BORDER, COLOR_RESET, COLOR_BORDER, COLOR_RESET);
    printf("  %s║%s         Double Down on any initial 2 cards • Split pairs for dual hands     %s║%s\n",
           COLOR_BORDER, COLOR_RESET, COLOR_BORDER, COLOR_RESET);
    printf("  %s║%s         Late Surrender available on initial 2 cards (reclaims 50%% bet)     %s║%s\n",
           COLOR_BORDER, COLOR_RESET, COLOR_BORDER, COLOR_RESET);
    printf("  %s║%s                                                                           %s║%s\n",
           COLOR_BORDER, COLOR_RESET, COLOR_BORDER, COLOR_RESET);
  } else {
    for (int r = 0; r < 5; r++) {
      printf("  %s║%s    ", COLOR_BORDER, COLOR_RESET);
      int chars_printed = 0;
      for (int i = 0; i < game->dealer_hand.count; i++) {
        bool face_down = (i == 1 && !game->dealer_hole_revealed);
        char c_buf[128];
        get_card_line(game->dealer_hand.cards[i], face_down, r, c_buf, sizeof(c_buf));
        printf("%s ", c_buf);
        chars_printed += 8;
      }
      int rem = 71 - chars_printed;
      if (rem > 0) {
        printf("%*s", rem, "");
      }
      printf("%s║%s\n", COLOR_BORDER, COLOR_RESET);
    }
  }

  // Line 10: Divider
  printf("  %s╟───────────────────────────────────────────────────────────────────────────╢%s\n",
         COLOR_BORDER_DIM, COLOR_RESET);

  // Line 11: Player header
  char player_info[128];
  if (game->state == STATE_BETTING) {
    snprintf(player_info, sizeof(player_info), "PLAYER'S HAND");
  } else if (game->player_hand_count == 1) {
    bool is_soft = false;
    int p_tot = hand_calculate_total(&game->player_hands[0], &is_soft);
    const char *status_tag = "";
    if (game->player_hands[0].is_blackjack) status_tag = " - BLACKJACK!";
    else if (game->player_hands[0].is_busted) status_tag = " - BUST!";
    else if (game->player_hands[0].is_standing) status_tag = " - STAND";

    snprintf(player_info, sizeof(player_info), "PLAYER'S HAND   [Total: %d%s]  [Bet: $%d]%s",
             p_tot, is_soft ? " (Soft)" : "", game->player_hands[0].bet, status_tag);
  } else {
    // 2 Split hands
    int t0 = hand_calculate_total(&game->player_hands[0], NULL);
    int t1 = hand_calculate_total(&game->player_hands[1], NULL);
    snprintf(player_info, sizeof(player_info),
             "HAND 1: %s[%d]%s [Bet: $%d]%s  |  HAND 2: %s[%d]%s [Bet: $%d]%s",
             (game->active_hand_idx == 0 && game->state == STATE_PLAYER_TURN) ? "▶" : "",
             t0, game->player_hands[0].is_busted ? " (BUST)" : "",
             game->player_hands[0].bet,
             (game->active_hand_idx == 0 && game->state == STATE_PLAYER_TURN) ? " ◀" : "",
             (game->active_hand_idx == 1 && game->state == STATE_PLAYER_TURN) ? "▶" : "",
             t1, game->player_hands[1].is_busted ? " (BUST)" : "",
             game->player_hands[1].bet,
             (game->active_hand_idx == 1 && game->state == STATE_PLAYER_TURN) ? " ◀" : "");
  }
  printf("  %s║%s  %-71s  %s║%s\n", COLOR_BORDER, COLOR_RESET, player_info, COLOR_BORDER, COLOR_RESET);

  // Lines 12-16: Player cards (or Chip selector in betting phase)
  if (game->state == STATE_BETTING) {
    printf("  %s║%s                    %s[ PLACE YOUR BET FOR NEXT HAND ]%s                        %s║%s\n",
           COLOR_BORDER, COLOR_RESET, COLOR_CYAN, COLOR_RESET, COLOR_BORDER, COLOR_RESET);
    char bet_str[32];
    snprintf(bet_str, sizeof(bet_str), "$%d", game->current_bet);
    int bet_len = (int)strlen(bet_str);
    int pad_r = 33 - bet_len;
    if (pad_r < 0) pad_r = 0;
    printf("  %s║%s                            Current Bet: %s%s%s%*s%s║%s\n",
           COLOR_BORDER, COLOR_RESET, COLOR_GOLD, bet_str, COLOR_RESET,
           pad_r, "",
           COLOR_BORDER, COLOR_RESET);
    printf("  %s║%s          Presets: [1] $10   [2] $25   [3] $50   [4] $100   [5] $500          %s║%s\n",
           COLOR_BORDER, COLOR_RESET, COLOR_BORDER, COLOR_RESET);
    printf("  %s║%s          [↑/W] +$10   [↓/S] -$10   [A] All-In ($%d)                           %s║%s\n",
           COLOR_BORDER, COLOR_RESET, game->chips, COLOR_BORDER, COLOR_RESET);
    printf("  %s║%s                  Press %s[Space]%s or %s[Enter]%s to DEAL CARDS                 %s║%s\n",
           COLOR_BORDER, COLOR_RESET, COLOR_GOLD, COLOR_RESET, COLOR_GOLD, COLOR_RESET, COLOR_BORDER, COLOR_RESET);
  } else if (game->player_hand_count == 1) {
    // Single hand cards
    for (int r = 0; r < 5; r++) {
      printf("  %s║%s    ", COLOR_BORDER, COLOR_RESET);
      int chars_printed = 0;
      for (int i = 0; i < game->player_hands[0].count; i++) {
        char c_buf[128];
        get_card_line(game->player_hands[0].cards[i], false, r, c_buf, sizeof(c_buf));
        printf("%s ", c_buf);
        chars_printed += 8;
      }
      int rem = 71 - chars_printed;
      if (rem > 0) {
        printf("%*s", rem, "");
      }
      printf("%s║%s\n", COLOR_BORDER, COLOR_RESET);
    }
  } else {
    // 2 Split hands rendered side by side
    for (int r = 0; r < 5; r++) {
      printf("  %s║%s  ", COLOR_BORDER, COLOR_RESET);
      int chars_h0 = 0;
      for (int i = 0; i < game->player_hands[0].count; i++) {
        char c_buf[128];
        get_card_line(game->player_hands[0].cards[i], false, r, c_buf, sizeof(c_buf));
        printf("%s ", c_buf);
        chars_h0 += 8;
      }
      int pad0 = 33 - chars_h0;
      if (pad0 > 0) printf("%*s", pad0, "");

      printf("%s│%s  ", COLOR_BORDER_DIM, COLOR_RESET);

      int chars_h1 = 0;
      for (int i = 0; i < game->player_hands[1].count; i++) {
        char c_buf[128];
        get_card_line(game->player_hands[1].cards[i], false, r, c_buf, sizeof(c_buf));
        printf("%s ", c_buf);
        chars_h1 += 8;
      }
      int pad1 = 34 - chars_h1;
      if (pad1 > 0) printf("%*s", pad1, "");

      printf("%s║%s\n", COLOR_BORDER, COLOR_RESET);
    }
  }

  // Line 17: Result / Status banner
  const char *b_col = COLOR_CYAN;
  if (game->banner_color == 1) b_col = COLOR_WIN;
  else if (game->banner_color == 2) b_col = COLOR_LOSE;
  else if (game->banner_color == 3) b_col = COLOR_PUSH;
  else if (game->banner_color == 4) b_col = COLOR_BJ;

  const char *msg = (game->banner_msg[0] != '\0') ? game->banner_msg : game->status_msg;
  int msg_len = (int)strlen(msg);
  int pad_total = 73 - msg_len;
  if (pad_total < 0) pad_total = 0;
  int pad_left = pad_total / 2;
  int pad_right = pad_total - pad_left;

  printf("  %s║%s %*s%s%s%s%*s %s║%s\n",
         COLOR_BORDER, COLOR_RESET,
         pad_left, "",
         b_col, msg, COLOR_RESET,
         pad_right, "",
         COLOR_BORDER, COLOR_RESET);

  // Line 18: Bottom table border
  printf("  %s╚═══════════════════════════════════════════════════════════════════════════╝%s\n",
         COLOR_BORDER, COLOR_RESET);

  // Line 19: Action Controls bar
  if (game->state == STATE_BETTING) {
    printf("  %s[↑/↓/W/S]%s ±$10  %s[1-5]%s Preset  %s[A]%s All-In  %s[Space/Enter]%s Deal  %s[Q]%s Exit\n",
           COLOR_GOLD, COLOR_RESET, COLOR_GOLD, COLOR_RESET, COLOR_CYAN, COLOR_RESET, COLOR_GOLD, COLOR_RESET, COLOR_DIM, COLOR_RESET);
  } else if (game->state == STATE_INSURANCE) {
    printf("  %s[Y/I]%s Take Insurance ($%d, pays 2:1)  %s[N/Space/Enter]%s Decline  %s[Q]%s Exit\n",
           COLOR_GOLD, COLOR_RESET, game->current_bet / 2, COLOR_CYAN, COLOR_RESET, COLOR_DIM, COLOR_RESET);
  } else if (game->state == STATE_PLAYER_TURN) {
    const Hand *h = &game->player_hands[game->active_hand_idx];
    bool can_d = hand_can_double(h, game->chips);
    bool can_s = hand_can_split(h, game->chips);
    bool can_surr = hand_can_surrender(h, game->state);

    printf("  %s[H/Space]%s Hit  %s[S/Enter]%s Stand%s%s%s%s%s%s  %s[Q]%s Exit\n",
           COLOR_GOLD, COLOR_RESET, COLOR_CYAN, COLOR_RESET,
           can_d ? "  \033[1;38;2;255;215;60m[D]\033[0m Double" : "", "",
           can_s ? "  \033[1;38;2;255;215;60m[P]\033[0m Split" : "", "",
           can_surr ? "  \033[1;38;2;255;215;60m[U]\033[0m Surrender" : "", "",
           COLOR_DIM, COLOR_RESET);
  } else if (game->state == STATE_ROUND_OVER) {
    printf("  %s[Space/Enter]%s Next Hand  %s[R]%s Reload Bankroll  %s[Q/Esc]%s Exit to Arcade\n",
           COLOR_GOLD, COLOR_RESET, COLOR_CYAN, COLOR_RESET, COLOR_DIM, COLOR_RESET);
  } else {
    // GAME OVER
    printf("  %s[R/Space/Enter]%s Reload $%d Bankroll  %s[Q/Esc]%s Exit to Arcade\n",
           COLOR_GOLD, COLOR_RESET, RELOAD_AMOUNT, COLOR_DIM, COLOR_RESET);
  }

  // Line 20: Stats summary bar
  float win_pct = (game->hands_played > 0) ? ((float)game->hands_won * 100.0f / (float)game->hands_played) : 0.0f;
  printf("  %sHands: %d | Won: %d (%.1f%%) | High: $%d | Shoe: %d/%d cards | Min: $%d%s\n",
         COLOR_DIM,
         game->hands_played, game->hands_won, win_pct, game->high_chips,
         SHOE_SIZE - game->shoe_index, SHOE_SIZE, MIN_BET,
         COLOR_RESET);

  fflush(stdout);
}
