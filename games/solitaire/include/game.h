#ifndef GAME_SOLITAIRE_H
#define GAME_SOLITAIRE_H

#include <stdbool.h>
#include <stddef.h>
#include "config.h"

typedef struct {
  Suit suit;
  Rank rank;
  bool face_up;
} Card;

typedef struct {
  Card cards[TOTAL_CARDS];
  int count;
} CardPile;

typedef struct {
  Card stock[TOTAL_CARDS];
  int stock_count;
  Card waste[TOTAL_CARDS];
  int waste_count;
  Card foundations[NUM_FOUNDATIONS][TOTAL_CARDS];
  int foundation_count[NUM_FOUNDATIONS];
  Card tableau[NUM_TABLEAU][MAX_TABLEAU_CARDS];
  int tableau_count[NUM_TABLEAU];
  int score;
  int moves;
} GameUndoState;

typedef struct {
  CardPile stock;
  CardPile waste;
  CardPile foundations[NUM_FOUNDATIONS];
  CardPile tableau[NUM_TABLEAU];

  PileType selected_pile;
  int selected_card_idx;

  PileType cursor_pile;
  int cursor_card_idx;

  GameStateStatus status;
  int score;
  int moves;
  unsigned int seed;
  char status_msg[80];

  int high_score;
  int games_won;
  int games_played;

  GameUndoState undo_stack[MAX_UNDO_STEPS];
  int undo_count;

  // Win animation / celebration
  bool won_celebration;
  int win_anim_ticks;
} GameState;

// Card logic helpers
bool card_is_red(Card c);
bool card_is_black(Card c);
bool card_can_stack_tableau(Card card, Card target_top);
bool card_can_stack_foundation(Card card, const CardPile *found);
const char *rank_name(Rank r);
const char *suit_symbol(Suit s);

// Core game functions
void game_init(GameState *game, unsigned int seed);
void game_restart(GameState *game);
bool game_draw_stock(GameState *game);
bool game_move(GameState *game, PileType src, int card_idx, PileType dst);
bool game_auto_move(GameState *game);
bool game_auto_foundation_for_pile(GameState *game, PileType src);
bool game_undo(GameState *game);
bool game_get_hint(const GameState *game, char *out_hint, size_t buf_size);
void game_check_win(GameState *game);
bool game_is_all_exposed(const GameState *game);
void game_save_scores(const GameState *game);

// Selection and cursor helpers
void game_select_pile(GameState *game, PileType pile);
void game_select_tableau_card(GameState *game, int tableau_idx, int card_idx);
void game_clear_selection(GameState *game);
void game_nav_cursor(GameState *game, GameAction nav_act);

#endif /* GAME_SOLITAIRE_H */

