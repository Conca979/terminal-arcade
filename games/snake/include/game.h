#ifndef GAME_SNAKE_H
#define GAME_SNAKE_H

#include "config.h"

typedef struct {
  int x;
  int y;
} Position;

typedef struct {
  Position pos;
  FoodType type;
  unsigned long long spawn_time;
  unsigned long long expire_time;
} FoodItem;

typedef struct {
  Position body[MAX_SNAKE_LEN];
  int length;
  Direction dir;
  Direction next_dir;

  FoodItem regular_food;
  FoodItem bonus_food;

  // Power-up Buffs
  bool overclock_active;
  unsigned long long overclock_expire;
  bool phase_active;
  unsigned long long phase_expire;

  bool wrap_mode;
  int score;
  int high_score;

  PlayState state;
  unsigned long long last_move_time;
  unsigned long long next_bonus_spawn_time;
} GameState;

void game_init(GameState *game);
void game_reset(GameState *game);
void game_update(GameState *game, unsigned long long current_time);
void game_handle_action(GameState *game, GameAction action, unsigned long long current_time);
int game_get_tick_interval(const GameState *game);
void game_load_high_score(GameState *game);
void game_save_high_score(const GameState *game);

#endif /* GAME_SNAKE_H */
