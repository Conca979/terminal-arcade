#ifndef GAME_WORDLE_H
#define GAME_WORDLE_H

#include <stdbool.h>
#include <stdint.h>
#include "config.h"

typedef struct {
  char letters[WORD_LEN + 1];
  TileStatus status[WORD_LEN];
} GuessRow;

typedef struct {
  char target_word[WORD_LEN + 1];
  GuessRow guesses[MAX_ATTEMPTS];
  int current_attempt;
  int current_letter_idx;
  KeyStatus key_status[26];
  PlayState state;
  char message[64];
  int message_timer_ms;
  int games_played;
  int games_won;
  int current_streak;
  int max_streak;
  int guess_distribution[MAX_ATTEMPTS];
} GameState;

void game_init(GameState *game);
void game_new_round(GameState *game);
void game_set_target_word(GameState *game, const char *word);
void game_handle_action(GameState *game, GameAction action, char letter);
void game_update(GameState *game, float dt);
bool game_submit_guess(GameState *game);
void game_evaluate_guess(const char *guess, const char *target, TileStatus *out_status);
bool game_is_valid_word(const char *word);
int game_load_valid_dictionary(void);

void game_save(const GameState *game);
bool game_load(GameState *game);
void game_delete_save(void);

void game_load_stats(GameState *game);
void game_save_stats(const GameState *game);

#endif /* GAME_WORDLE_H */
