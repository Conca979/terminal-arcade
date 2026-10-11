#ifndef BREAKER_GAME_H
#define BREAKER_GAME_H

#include <stdbool.h>
#include <stdint.h>
#include "config.h"

typedef enum {
  ACT_NONE,
  ACT_AIM_LEFT,
  ACT_AIM_RIGHT,
  ACT_MOUSE_MOVE,
  ACT_FIRE,
  ACT_TURBO,
  ACT_RESTART,
  ACT_QUIT
} GameAction;

typedef enum {
  PHASE_AIMING,
  PHASE_FIRING,
  PHASE_GAME_OVER,
  PHASE_EXIT
} PlayPhase;

typedef enum {
  BRICK_NONE = 0,
  BRICK_NORMAL,
  BRICK_ADD_BALL,
  BRICK_BOMB,
  BRICK_LASER_ROW,
  BRICK_LASER_COL
} BrickType;

typedef struct {
  BrickType type;
  int hp;
  int max_hp;
  int flash_timer;
} Brick;

typedef struct {
  float x;
  float y;
  float vx;
  float vy;
  bool active;
  bool landed;
} Ball;

typedef struct {
  float x;
  float y;
} Vec2;

typedef struct {
  Vec2 points[64];
  int count;
} Trajectory;

typedef struct {
  Brick grid[BRICK_ROWS][BRICK_COLS];
  Ball balls[MAX_BALLS];

  int total_balls;
  int balls_to_launch;
  int balls_active;
  int collected_balls;

  float launcher_x;
  float next_launcher_x;
  bool first_ball_landed;

  float aim_angle; // In radians (-PI to 0)
  Trajectory laser_guide;

  PlayPhase phase;
  bool turbo_mode;
  int round;
  int score;
  int high_score;
  bool resumed_game;

  unsigned long long last_launch_time;
  int mouse_term_x;
  int mouse_term_y;
  bool mouse_aiming;
} GameState;

void game_init(GameState *game);
void game_reset(GameState *game);
void game_save(const GameState *game);
bool game_load(GameState *game);
void game_delete_save(void);

void game_handle_action(GameState *game, GameAction action, int param1, int param2, unsigned long long current_time);
void game_update(GameState *game, unsigned long long current_time);
void game_set_aim_angle(GameState *game, float angle_rad);
void game_set_aim_target(GameState *game, float target_x, float target_y);
void game_fire(GameState *game, unsigned long long current_time);

void game_spawn_new_row(GameState *game);
void game_calculate_trajectory(GameState *game);

#endif // BREAKER_GAME_H
