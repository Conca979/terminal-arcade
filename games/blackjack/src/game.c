#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "scores.h"

int card_value(Card card) {
  if (card.rank >= RANK_2 && card.rank <= RANK_10) {
    return (int)card.rank;
  }
  if (card.rank >= RANK_J && card.rank <= RANK_K) {
    return 10;
  }
  if (card.rank == RANK_A) {
    return 11;
  }
  return 0;
}

int hand_calculate_total(const Hand *hand, bool *out_is_soft) {
  int sum = 0;
  int aces = 0;

  for (int i = 0; i < hand->count; i++) {
    int val = card_value(hand->cards[i]);
    sum += val;
    if (hand->cards[i].rank == RANK_A) {
      aces++;
    }
  }

  while (sum > 21 && aces > 0) {
    sum -= 10;
    aces--;
  }

  if (out_is_soft) {
    *out_is_soft = (aces > 0);
  }

  return sum;
}

bool hand_is_blackjack(const Hand *hand) {
  if (hand->count != 2 || hand->is_split) {
    return false;
  }
  return (hand_calculate_total(hand, NULL) == 21);
}

bool hand_can_split(const Hand *hand, int chips) {
  if (hand->count != 2 || hand->is_split || chips < hand->bet) {
    return false;
  }
  return (hand->cards[0].rank == hand->cards[1].rank ||
          card_value(hand->cards[0]) == card_value(hand->cards[1]));
}

bool hand_can_double(const Hand *hand, int chips) {
  if (hand->count != 2 || hand->is_doubled || chips < hand->bet) {
    return false;
  }
  return true;
}

bool hand_can_surrender(const Hand *hand, RoundState state) {
  if (state != STATE_PLAYER_TURN || hand->count != 2 || hand->is_split || hand->is_doubled) {
    return false;
  }
  return true;
}

void game_init_shoe(GameState *game) {
  int idx = 0;
  for (int d = 0; d < NUM_DECKS; d++) {
    for (int s = 0; s < 4; s++) {
      for (int r = (int)RANK_2; r <= (int)RANK_A; r++) {
        game->shoe[idx].suit = (Suit)s;
        game->shoe[idx].rank = (Rank)r;
        idx++;
      }
    }
  }

  int jitter = (rand() % 31) - 15;
  game->cut_card_index = CUT_CARD_THRESHOLD + jitter;
  game_shuffle_shoe(game);
}

void game_shuffle_shoe(GameState *game) {
  for (int i = SHOE_SIZE - 1; i > 0; i--) {
    int j = rand() % (i + 1);
    Card temp = game->shoe[i];
    game->shoe[i] = game->shoe[j];
    game->shoe[j] = temp;
  }
  game->shoe_index = 0;
  game->need_shuffle = false;
}

Card game_draw_card(GameState *game) {
  if (game->shoe_index >= SHOE_SIZE) {
    game_shuffle_shoe(game);
  }
  Card card = game->shoe[game->shoe_index++];
  if (game->shoe_index >= game->cut_card_index) {
    game->need_shuffle = true;
  }
  return card;
}

void game_load_scores(GameState *game) {
  int c = arcade_score_get("blackjack", "chips");
  int h = arcade_score_get("blackjack", "high_chips");
  int played = arcade_score_get("blackjack", "hands_played");
  int won = arcade_score_get("blackjack", "hands_won");

  game->chips = (c > 0) ? c : STARTING_BANKROLL;
  game->high_chips = (h > game->chips) ? h : game->chips;
  game->hands_played = played;
  game->hands_won = won;
}

void game_save_scores(const GameState *game) {
  arcade_score_set("blackjack", "chips", game->chips);
  arcade_score_set("blackjack", "high_chips", game->high_chips);
  arcade_score_set("blackjack", "hands_played", game->hands_played);
  arcade_score_set("blackjack", "hands_won", game->hands_won);
}

void game_reset_bankroll(GameState *game) {
  game->chips = STARTING_BANKROLL;
  if (game->chips > game->high_chips) {
    game->high_chips = game->chips;
  }
  game->current_bet = DEFAULT_BET;
  game->state = STATE_BETTING;
  snprintf(game->banner_msg, sizeof(game->banner_msg), "Bankroll reloaded to $%d!", STARTING_BANKROLL);
  game->banner_color = 1;
  game_save_scores(game);
}

void game_init(GameState *game) {
  memset(game, 0, sizeof(GameState));
  srand((unsigned int)time(NULL));

  game_init_shoe(game);
  game_load_scores(game);

  game->current_bet = (game->chips >= DEFAULT_BET) ? DEFAULT_BET : MIN_BET;
  if (game->chips < MIN_BET) {
    game->chips = STARTING_BANKROLL;
    game->current_bet = DEFAULT_BET;
    if (game->chips > game->high_chips) {
      game->high_chips = game->chips;
    }
  }

  game->state = STATE_BETTING;
  snprintf(game->status_msg, sizeof(game->status_msg), "Place your bet and press [Space/Enter] to deal!");
}

void game_start_round(GameState *game) {
  if (game->need_shuffle) {
    game_shuffle_shoe(game);
  }

  if (game->chips < MIN_BET && game->current_bet > game->chips) {
    game->state = STATE_GAME_OVER;
    snprintf(game->banner_msg, sizeof(game->banner_msg), "OUT OF CHIPS! Press [R] to reload $%d bankroll.", RELOAD_AMOUNT);
    game->banner_color = 2;
    return;
  }

  if (game->current_bet > game->chips) {
    game->current_bet = game->chips;
  }

  game->chips -= game->current_bet;

  // Clear hands
  memset(&game->dealer_hand, 0, sizeof(Hand));
  memset(game->player_hands, 0, sizeof(game->player_hands));
  game->player_hand_count = 1;
  game->active_hand_idx = 0;
  game->dealer_hole_revealed = false;
  game->insurance_offered = false;
  game->insurance_bought = false;
  game->insurance_bet = 0;
  game->banner_msg[0] = '\0';
  game->banner_color = 0;

  // Set initial player bet
  game->player_hands[0].bet = game->current_bet;

  // Deal 2 cards to player, 2 to dealer (1 face down)
  game->player_hands[0].cards[0] = game_draw_card(game);
  game->dealer_hand.cards[0] = game_draw_card(game);
  game->player_hands[0].cards[1] = game_draw_card(game);
  game->dealer_hand.cards[1] = game_draw_card(game);
  game->player_hands[0].count = 2;
  game->dealer_hand.count = 2;

  // Check if dealer upcard is Ace
  if (game->dealer_hand.cards[0].rank == RANK_A) {
    game->insurance_offered = true;
    game->state = STATE_INSURANCE;
    snprintf(game->status_msg, sizeof(game->status_msg), "Dealer shows Ace! Take Insurance for $%d? [Y/I] Yes  [N/Enter] No", game->current_bet / 2);
    return;
  }

  // Check if dealer upcard is 10-value card (dealer peeks for Blackjack)
  if (card_value(game->dealer_hand.cards[0]) == 10) {
    bool dealer_bj = hand_is_blackjack(&game->dealer_hand);
    bool player_bj = hand_is_blackjack(&game->player_hands[0]);

    if (dealer_bj) {
      game->dealer_hole_revealed = true;
      game->hands_played++;
      if (player_bj) {
        game->player_hands[0].result = RESULT_PUSH;
        game->chips += game->current_bet;
        game->rounds_tied++;
        game->banner_color = 3;
        snprintf(game->banner_msg, sizeof(game->banner_msg), "BOTH HAVE BLACKJACK! PUSH ($%d RETURNED)", game->current_bet);
      } else {
        game->player_hands[0].result = RESULT_LOSE;
        game->banner_color = 2;
        snprintf(game->banner_msg, sizeof(game->banner_msg), "DEALER HAS BLACKJACK! YOU LOST $%d", game->current_bet);
      }
      game_save_scores(game);
      game->state = STATE_ROUND_OVER;
      return;
    }
  }

  // If dealer does not have Blackjack, check player Blackjack
  if (hand_is_blackjack(&game->player_hands[0])) {
    game->dealer_hole_revealed = true;
    game->player_hands[0].is_blackjack = true;
    game->player_hands[0].result = RESULT_BLACKJACK;
    int profit = (game->current_bet * 3) / 2;
    int total_payout = game->current_bet + profit;
    game->chips += total_payout;
    game->player_hands[0].payout = total_payout;
    if (game->chips > game->high_chips) {
      game->high_chips = game->chips;
    }
    game->hands_played++;
    game->hands_won++;
    game->banner_color = 4;
    snprintf(game->banner_msg, sizeof(game->banner_msg), "NATURAL BLACKJACK! PAYS 3:2! YOU WON +$%d!", profit);
    game_save_scores(game);
    game->state = STATE_ROUND_OVER;
    return;
  }

  // Normal player turn
  game->state = STATE_PLAYER_TURN;
  snprintf(game->status_msg, sizeof(game->status_msg), "Your move: [H] Hit  [S] Stand%s%s",
           hand_can_double(&game->player_hands[0], game->chips) ? "  [D] Double" : "",
           hand_can_split(&game->player_hands[0], game->chips) ? "  [P] Split" : "");
}

void game_player_insurance(GameState *game, bool accept) {
  if (game->state != STATE_INSURANCE) {
    return;
  }

  int ins_cost = game->current_bet / 2;
  if (accept && game->chips >= ins_cost) {
    game->chips -= ins_cost;
    game->insurance_bet = ins_cost;
    game->insurance_bought = true;
  } else {
    game->insurance_bought = false;
  }

  bool dealer_bj = hand_is_blackjack(&game->dealer_hand);
  bool player_bj = hand_is_blackjack(&game->player_hands[0]);

  if (dealer_bj) {
    game->dealer_hole_revealed = true;
    game->hands_played++;

    if (game->insurance_bought) {
      int ins_payout = game->insurance_bet * 3;
      game->chips += ins_payout;
    }

    if (player_bj) {
      game->player_hands[0].result = RESULT_PUSH;
      game->chips += game->current_bet;
      game->rounds_tied++;
      game->banner_color = 3;
      snprintf(game->banner_msg, sizeof(game->banner_msg), "DEALER HAS BLACKJACK! PUSH ON MAIN BET%s",
               game->insurance_bought ? " + INSURANCE PAID 2:1!" : ".");
    } else {
      game->player_hands[0].result = RESULT_LOSE;
      game->banner_color = game->insurance_bought ? 3 : 2;
      snprintf(game->banner_msg, sizeof(game->banner_msg), "DEALER HAS BLACKJACK!%s",
               game->insurance_bought ? " INSURANCE PAID 2:1 (EVEN MONEY NET)" : " YOU LOST.");
    }

    game_save_scores(game);
    game->state = STATE_ROUND_OVER;
    return;
  }

  // Dealer does not have Blackjack
  if (player_bj) {
    game->dealer_hole_revealed = true;
    game->player_hands[0].is_blackjack = true;
    game->player_hands[0].result = RESULT_BLACKJACK;
    int profit = (game->current_bet * 3) / 2;
    int total_payout = game->current_bet + profit;
    game->chips += total_payout;
    game->player_hands[0].payout = total_payout;
    if (game->chips > game->high_chips) {
      game->high_chips = game->chips;
    }
    game->hands_played++;
    game->hands_won++;
    game->banner_color = 4;
    snprintf(game->banner_msg, sizeof(game->banner_msg), "NATURAL BLACKJACK! YOU WON +$%d%s",
             profit, game->insurance_bought ? " (Insurance lost)" : "!");
    game_save_scores(game);
    game->state = STATE_ROUND_OVER;
    return;
  }

  // Play continues
  game->state = STATE_PLAYER_TURN;
  snprintf(game->status_msg, sizeof(game->status_msg), "Dealer checked hole card: No Blackjack. Your move!");
}

void game_player_hit(GameState *game) {
  if (game->state != STATE_PLAYER_TURN) {
    return;
  }

  Hand *hand = &game->player_hands[game->active_hand_idx];
  if (hand->count >= MAX_HAND_CARDS) {
    return;
  }

  hand->cards[hand->count++] = game_draw_card(game);
  int total = hand_calculate_total(hand, NULL);

  if (total > 21) {
    hand->is_busted = true;
    hand->result = RESULT_BUST;
    // Advance to next hand or dealer turn
    if (game->active_hand_idx + 1 < game->player_hand_count) {
      game->active_hand_idx++;
      snprintf(game->status_msg, sizeof(game->status_msg), "Hand 1 busted! Playing Hand 2...");
    } else {
      // Check if all player hands busted
      bool all_busted = true;
      for (int i = 0; i < game->player_hand_count; i++) {
        if (!game->player_hands[i].is_busted) {
          all_busted = false;
          break;
        }
      }
      if (all_busted) {
        game->dealer_hole_revealed = true;
        game_evaluate_results(game);
      } else {
        game_dealer_play(game);
      }
    }
  } else if (total == 21) {
    // Auto-stand on 21
    hand->is_standing = true;
    if (game->active_hand_idx + 1 < game->player_hand_count) {
      game->active_hand_idx++;
      snprintf(game->status_msg, sizeof(game->status_msg), "Hand 1 got 21! Playing Hand 2...");
    } else {
      game_dealer_play(game);
    }
  }
}

void game_player_stand(GameState *game) {
  if (game->state != STATE_PLAYER_TURN) {
    return;
  }

  Hand *hand = &game->player_hands[game->active_hand_idx];
  hand->is_standing = true;

  if (game->active_hand_idx + 1 < game->player_hand_count) {
    game->active_hand_idx++;
    snprintf(game->status_msg, sizeof(game->status_msg), "Playing Hand 2...");
  } else {
    game_dealer_play(game);
  }
}

void game_player_double(GameState *game) {
  if (game->state != STATE_PLAYER_TURN) {
    return;
  }

  Hand *hand = &game->player_hands[game->active_hand_idx];
  if (!hand_can_double(hand, game->chips)) {
    return;
  }

  game->chips -= hand->bet;
  hand->bet *= 2;
  hand->is_doubled = true;

  // Double down receives exactly one card
  hand->cards[hand->count++] = game_draw_card(game);
  int total = hand_calculate_total(hand, NULL);

  if (total > 21) {
    hand->is_busted = true;
    hand->result = RESULT_BUST;
  } else {
    hand->is_standing = true;
  }

  if (game->active_hand_idx + 1 < game->player_hand_count) {
    game->active_hand_idx++;
    snprintf(game->status_msg, sizeof(game->status_msg), "Playing Hand 2...");
  } else {
    bool all_busted = true;
    for (int i = 0; i < game->player_hand_count; i++) {
      if (!game->player_hands[i].is_busted) {
        all_busted = false;
        break;
      }
    }
    if (all_busted) {
      game->dealer_hole_revealed = true;
      game_evaluate_results(game);
    } else {
      game_dealer_play(game);
    }
  }
}

void game_player_split(GameState *game) {
  if (game->state != STATE_PLAYER_TURN) {
    return;
  }

  Hand *hand0 = &game->player_hands[0];
  if (!hand_can_split(hand0, game->chips)) {
    return;
  }

  game->chips -= hand0->bet;

  // Split into Hand 0 and Hand 1
  Hand *hand1 = &game->player_hands[1];
  hand1->cards[0] = hand0->cards[1];
  hand1->count = 1;
  hand1->bet = hand0->bet;
  hand1->is_split = true;

  hand0->count = 1;
  hand0->is_split = true;
  game->player_hand_count = 2;

  // Deal second card to both hands
  hand0->cards[hand0->count++] = game_draw_card(game);
  hand1->cards[hand1->count++] = game_draw_card(game);

  // If split Aces, player receives 1 card per ace and stands automatically
  if (hand0->cards[0].rank == RANK_A) {
    hand0->is_standing = true;
    hand1->is_standing = true;
    snprintf(game->status_msg, sizeof(game->status_msg), "Split Aces: 1 card dealt to each hand.");
    game_dealer_play(game);
    return;
  }

  snprintf(game->status_msg, sizeof(game->status_msg), "Hands split! Playing Hand 1...");
}

void game_player_surrender(GameState *game) {
  if (game->state != STATE_PLAYER_TURN) {
    return;
  }

  Hand *hand = &game->player_hands[0];
  if (!hand_can_surrender(hand, game->state)) {
    return;
  }

  int refund = hand->bet / 2;
  game->chips += refund;
  hand->is_surrendered = true;
  hand->result = RESULT_SURRENDER;
  game->dealer_hole_revealed = true;

  game->banner_color = 3;
  snprintf(game->banner_msg, sizeof(game->banner_msg), "SURRENDERED: RECLAIMED 50%% ($%d) OF BET", refund);

  game->hands_played++;
  game_save_scores(game);
  game->state = STATE_ROUND_OVER;
}

void game_dealer_play(GameState *game) {
  game->state = STATE_DEALER_TURN;
  game->dealer_hole_revealed = true;

  // Dealer must hit until total >= 17 (Dealer stands on all 17s)
  while (hand_calculate_total(&game->dealer_hand, NULL) < 17 && game->dealer_hand.count < MAX_HAND_CARDS) {
    game->dealer_hand.cards[game->dealer_hand.count++] = game_draw_card(game);
  }

  int dealer_total = hand_calculate_total(&game->dealer_hand, NULL);
  if (dealer_total > 21) {
    game->dealer_hand.is_busted = true;
  }

  game_evaluate_results(game);
}

void game_evaluate_results(GameState *game) {
  int dealer_total = hand_calculate_total(&game->dealer_hand, NULL);
  bool dealer_busted = game->dealer_hand.is_busted;

  int net_round_profit = 0;
  int wins = 0;
  int pushes = 0;
  int losses = 0;

  for (int i = 0; i < game->player_hand_count; i++) {
    Hand *hand = &game->player_hands[i];
    if (hand->is_surrendered) {
      losses++;
      continue;
    }

    if (hand->is_busted) {
      hand->result = RESULT_BUST;
      hand->payout = 0;
      losses++;
      net_round_profit -= hand->bet;
      continue;
    }

    int player_total = hand_calculate_total(hand, NULL);

    if (dealer_busted) {
      hand->result = RESULT_WIN;
      hand->payout = hand->bet * 2;
      game->chips += hand->payout;
      wins++;
      net_round_profit += hand->bet;
    } else if (player_total > dealer_total) {
      hand->result = RESULT_WIN;
      hand->payout = hand->bet * 2;
      game->chips += hand->payout;
      wins++;
      net_round_profit += hand->bet;
    } else if (player_total < dealer_total) {
      hand->result = RESULT_LOSE;
      hand->payout = 0;
      losses++;
      net_round_profit -= hand->bet;
    } else {
      hand->result = RESULT_PUSH;
      hand->payout = hand->bet;
      game->chips += hand->payout;
      pushes++;
    }
  }

  if (game->chips > game->high_chips) {
    game->high_chips = game->chips;
  }

  game->hands_played += game->player_hand_count;
  game->hands_won += wins;
  game->rounds_tied += pushes;

  // Build summary banner
  if (game->player_hand_count == 1) {
    Hand *h = &game->player_hands[0];
    if (h->result == RESULT_WIN) {
      game->banner_color = 1;
      if (dealer_busted) {
        snprintf(game->banner_msg, sizeof(game->banner_msg), "DEALER BUSTED! YOU WON +$%d!", h->bet);
      } else {
        snprintf(game->banner_msg, sizeof(game->banner_msg), "YOU WON +$%d! (%d vs Dealer %d)", h->bet, hand_calculate_total(h, NULL), dealer_total);
      }
    } else if (h->result == RESULT_BUST) {
      game->banner_color = 2;
      snprintf(game->banner_msg, sizeof(game->banner_msg), "BUSTED! (%d) YOU LOST -$%d", hand_calculate_total(h, NULL), h->bet);
    } else if (h->result == RESULT_LOSE) {
      game->banner_color = 2;
      snprintf(game->banner_msg, sizeof(game->banner_msg), "DEALER WINS (%d vs %d) -$%d", dealer_total, hand_calculate_total(h, NULL), h->bet);
    } else if (h->result == RESULT_PUSH) {
      game->banner_color = 3;
      snprintf(game->banner_msg, sizeof(game->banner_msg), "PUSH AT %d! BET RETURNED ($%d)", dealer_total, h->bet);
    }
  } else {
    // Split round summary
    if (net_round_profit > 0) {
      game->banner_color = 1;
      snprintf(game->banner_msg, sizeof(game->banner_msg), "SPLIT ROUND WON: +$%d NET PROFIT! (%d W / %d L / %d P)", net_round_profit, wins, losses, pushes);
    } else if (net_round_profit < 0) {
      game->banner_color = 2;
      snprintf(game->banner_msg, sizeof(game->banner_msg), "SPLIT ROUND LOST: -$%d NET! (%d W / %d L / %d P)", -net_round_profit, wins, losses, pushes);
    } else {
      game->banner_color = 3;
      snprintf(game->banner_msg, sizeof(game->banner_msg), "SPLIT ROUND EVEN ($0 NET) (%d W / %d L / %d P)", wins, losses, pushes);
    }
  }

  game_save_scores(game);
  game->state = STATE_ROUND_OVER;
}

void game_handle_action(GameState *game, GameAction action) {
  if (action == ACT_QUIT) {
    game->should_quit = true;
    return;
  }

  if (action == ACT_RELOAD) {
    game_reset_bankroll(game);
    return;
  }

  if (game->state == STATE_BETTING) {
    switch (action) {
      case ACT_BET_INC:
        if (game->current_bet + 10 <= game->chips && game->current_bet + 10 <= MAX_BET) {
          game->current_bet += 10;
        }
        break;
      case ACT_BET_DEC:
        if (game->current_bet - 10 >= MIN_BET) {
          game->current_bet -= 10;
        }
        break;
      case ACT_BET_PRESET_1:
        if (10 <= game->chips) game->current_bet = 10;
        break;
      case ACT_BET_PRESET_2:
        if (25 <= game->chips) game->current_bet = 25;
        break;
      case ACT_BET_PRESET_3:
        if (50 <= game->chips) game->current_bet = 50;
        break;
      case ACT_BET_PRESET_4:
        if (100 <= game->chips) game->current_bet = 100;
        break;
      case ACT_BET_PRESET_5:
        if (500 <= game->chips) game->current_bet = 500;
        break;
      case ACT_BET_ALL_IN:
        if (game->chips > 0) game->current_bet = game->chips;
        break;
      case ACT_CONFIRM:
        game_start_round(game);
        break;
      default:
        break;
    }
  } else if (game->state == STATE_INSURANCE) {
    if (action == ACT_INSURE_YES) {
      game_player_insurance(game, true);
    } else if (action == ACT_INSURE_NO || action == ACT_CONFIRM) {
      game_player_insurance(game, false);
    }
  } else if (game->state == STATE_PLAYER_TURN) {
    switch (action) {
      case ACT_HIT:
        game_player_hit(game);
        break;
      case ACT_STAND:
        game_player_stand(game);
        break;
      case ACT_DOUBLE:
        game_player_double(game);
        break;
      case ACT_SPLIT:
        game_player_split(game);
        break;
      case ACT_SURRENDER:
        game_player_surrender(game);
        break;
      default:
        break;
    }
  } else if (game->state == STATE_ROUND_OVER) {
    if (action == ACT_CONFIRM) {
      if (game->chips < MIN_BET) {
        game->state = STATE_GAME_OVER;
        snprintf(game->banner_msg, sizeof(game->banner_msg), "OUT OF CHIPS! Press [R] to reload $%d bankroll.", RELOAD_AMOUNT);
        game->banner_color = 2;
      } else {
        if (game->current_bet > game->chips) {
          game->current_bet = game->chips;
        }
        game->state = STATE_BETTING;
        game->banner_msg[0] = '\0';
        game->banner_color = 0;
        snprintf(game->status_msg, sizeof(game->status_msg), "Place bet with [↑/↓/1-5/A], press [Space/Enter] to deal!");
      }
    }
  } else if (game->state == STATE_GAME_OVER) {
    if (action == ACT_RELOAD || action == ACT_CONFIRM) {
      game_reset_bankroll(game);
    }
  }
}
