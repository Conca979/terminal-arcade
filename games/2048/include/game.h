#ifndef GAME_2048_H
#define GAME_2048_H

#include "config.h"

typedef struct {
  int board[GRID_SIZE][GRID_SIZE];
  int score;
} BoardSnapshot;

typedef struct {
  int board[GRID_SIZE][GRID_SIZE];
  int score;
  int high_score;
  int move_count;
  int last_score_gain;
  PlayState state;
  bool won_acknowledged;

  BoardSnapshot history[MAX_UNDO_STATES];
  int history_count;
} GameState;

void game_init(GameState *game);
void game_reset(GameState *game);
bool game_slide(GameState *game, MoveDirection dir);
bool game_can_move(const GameState *game);
bool game_has_won(const GameState *game);
bool game_undo(GameState *game);
void game_spawn_tile(GameState *game);
void game_load_high_score(GameState *game);
void game_save_high_score(const GameState *game);
void game_save(const GameState *game);
bool game_load(GameState *game);
void game_delete_save(void);

#endif /* GAME_2048_H */
