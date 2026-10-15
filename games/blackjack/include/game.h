#ifndef GAME_BLACKJACK_H
#define GAME_BLACKJACK_H

#include <stdbool.h>
#include "config.h"

typedef struct {
  Suit suit;
  Rank rank;
} Card;

typedef struct {
  Card cards[MAX_HAND_CARDS];
  int count;
  int bet;
  bool is_doubled;
  bool is_split;
  bool is_blackjack;
  bool is_busted;
  bool is_standing;
  bool is_surrendered;
  HandResult result;
  int payout;
} Hand;

typedef struct {
  Card shoe[SHOE_SIZE];
  int shoe_index;
  int cut_card_index;
  bool need_shuffle;

  Hand dealer_hand;
  Hand player_hands[MAX_SPLIT_HANDS];
  int player_hand_count;
  int active_hand_idx;

  int chips;
  int high_chips;
  int current_bet;
  int insurance_bet;
  bool insurance_offered;
  bool insurance_bought;

  int hands_played;
  int hands_won;
  int rounds_tied;

  RoundState state;
  bool dealer_hole_revealed;
  char status_msg[128];
  char banner_msg[128];
  int banner_color; // 1: green, 2: red, 3: yellow, 4: cyan

  bool should_quit;
} GameState;

void game_init(GameState *game);
void game_reset_bankroll(GameState *game);
void game_init_shoe(GameState *game);
void game_shuffle_shoe(GameState *game);
Card game_draw_card(GameState *game);

int card_value(Card card);
int hand_calculate_total(const Hand *hand, bool *out_is_soft);
bool hand_is_blackjack(const Hand *hand);
bool hand_can_split(const Hand *hand, int chips);
bool hand_can_double(const Hand *hand, int chips);
bool hand_can_surrender(const Hand *hand, RoundState state);

void game_start_round(GameState *game);
void game_player_hit(GameState *game);
void game_player_stand(GameState *game);
void game_player_double(GameState *game);
void game_player_split(GameState *game);
void game_player_surrender(GameState *game);
void game_player_insurance(GameState *game, bool accept);
void game_dealer_play(GameState *game);
void game_evaluate_results(GameState *game);
void game_handle_action(GameState *game, GameAction action);

void game_load_scores(GameState *game);
void game_save_scores(const GameState *game);

#endif /* GAME_BLACKJACK_H */
