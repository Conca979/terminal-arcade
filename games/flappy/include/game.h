#ifndef GAME_FLAPPY_H
#define GAME_FLAPPY_H

#include <stdbool.h>
#include <stdint.h>
#include "config.h"

typedef struct {
  float y;
  float vy;
  float flap_anim_timer;
} Bird;

typedef struct {
  float x;
  int gap_y;
  bool passed;
} Pipe;

typedef struct {
  float x;
  float y;
  float speed;
  int type;
} Cloud;

#define MAX_CLOUDS 5

typedef struct {
  Bird bird;
  Pipe pipes[MAX_PIPES];
  Cloud clouds[MAX_CLOUDS];
  float ground_offset;
  int score;
  int high_score;
  MedalTier medal;
  PlayState state;
  float state_timer;
  bool is_new_high;
  int flash_timer_ms;
} GameState;

void game_init(GameState *game);
void game_reset(GameState *game);
void game_handle_action(GameState *game, GameAction action);
void game_update(GameState *game, float dt);

bool game_check_collision(const GameState *game);
MedalTier game_get_medal(int score);

void game_load_high_score(GameState *game);
void game_save_high_score(const GameState *game);

void game_save(const GameState *game);
bool game_load(GameState *game);
void game_delete_save(void);

#endif /* GAME_FLAPPY_H */
