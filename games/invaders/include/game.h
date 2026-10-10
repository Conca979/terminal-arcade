#ifndef GAME_INVADERS_H
#define GAME_INVADERS_H

#include "config.h"

typedef struct {
  int x;
  int lives;
  unsigned long long last_fire_time;
} Player;

typedef struct {
  bool active;
  int x;
  int y;
  int dy;
} Bullet;

typedef struct {
  bool alive;
  int type; // 0: squid (top), 1: crab (mid), 2: octopus (bot)
} Alien;

typedef struct {
  int origin_x;
  int origin_y;
  int dir_x;
  int alive_count;
  int anim_frame; // 0 or 1
  unsigned long long last_move_time;
  unsigned long long last_bomb_time;
} Swarm;

typedef struct {
  int x;
  int y;
  int hp[BUNKER_PARTS]; // Each part has 0..BUNKER_MAX_HP
} Bunker;

typedef struct {
  bool active;
  int x;
  int y;
  int dir_x;
  int score_val;
} UFO;

typedef struct {
  bool active;
  int x;
  int y;
  unsigned long long expire_time;
} Explosion;

typedef struct {
  Player player;
  Alien aliens[ALIEN_ROWS][ALIEN_COLS];
  Swarm swarm;
  Bullet player_bullets[MAX_PLAYER_BULLETS];
  Bullet alien_bombs[MAX_ALIEN_BOMBS];
  Bunker bunkers[BUNKER_COUNT];
  UFO ufo;
  Explosion explosions[MAX_EXPLOSIONS];

  int score;
  int high_score;
  int wave;

  PlayState state;
  unsigned long long wave_clear_time;
  unsigned long long next_ufo_time;
} GameState;

void game_init(GameState *game);
void game_reset(GameState *game);
void game_next_wave(GameState *game);
void game_update(GameState *game, unsigned long long current_time);
void game_handle_action(GameState *game, GameAction action, unsigned long long current_time);
void game_fire_bullet(GameState *game, unsigned long long current_time);
void game_load_high_score(GameState *game);
void game_save_high_score(const GameState *game);

#endif /* GAME_INVADERS_H */
