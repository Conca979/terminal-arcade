#ifndef GAME_PONG_H
#define GAME_PONG_H

#include <stdbool.h>
#include <stdint.h>
#include "config.h"

typedef struct {
  float x;
  float y;
} TrailPoint;

typedef struct {
  float x;
  float y;
  float vx;
  float vy;
  TrailPoint trail[TRAIL_MAX];
  int trail_count;
} Ball;

typedef struct {
  float y;
  float vy;
  int hit_flash;
} Paddle;

typedef struct {
  Ball ball;
  Paddle player;
  Paddle cpu;
  int player_score;
  int cpu_score;
  int rally_count;
  int max_rally;
  int high_score;
  AiDifficulty difficulty;
  PlayState state;
  int serve_timer_ms;
  int scorer;
  unsigned long long last_update_ms;
} GameState;

void game_init(GameState *game);
void game_reset_match(GameState *game);
void game_serve_ball(GameState *game, int toward_dir);
void game_handle_action(GameState *game, GameAction action);
void game_update(GameState *game, float dt);

void game_load_high_score(GameState *game);
void game_save_high_score(const GameState *game);

void game_save(const GameState *game);
bool game_load(GameState *game);
void game_delete_save(void);

#endif /* GAME_PONG_H */
