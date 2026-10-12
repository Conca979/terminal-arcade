#ifndef GAME_TYPING_H
#define GAME_TYPING_H

#include <stdbool.h>
#include <stdint.h>
#include "config.h"

typedef struct {
  bool active;
  EnemyType type;
  char word[MAX_WORD_LEN];
  int word_len;
  int typed_len;
  float x;
  float y;
  float speed;
} Enemy;

typedef struct {
  bool active;
  float x;
  float y;
  int frame;
} Explosion;

typedef struct {
  bool active;
  float target_x;
  float target_y;
  int lifetime_ms;
} LaserBeam;

typedef struct {
  Enemy enemies[MAX_ACTIVE_ENEMIES];
  int target_enemy_idx;
  Explosion explosions[MAX_EXPLOSIONS];
  LaserBeam lasers[MAX_LASERS];

  int wave;
  int words_spawned_in_wave;
  int words_defeated_in_wave;
  int wave_intro_timer_ms;
  int spawn_timer_ms;

  int shield_hp;
  int score;
  int high_score;
  int max_wpm;
  int words_completed;
  int total_keystrokes;
  int correct_keystrokes;
  int current_combo;
  int max_combo;

  unsigned long long start_time_ms;
  unsigned long long elapsed_active_ms;
  int emp_flash_ms;
  int shield_flash_ms;

  PlayState state;
} GameState;

void game_init(GameState *game);
void game_reset_mission(GameState *game);
void game_handle_action(GameState *game, GameAction action);
void game_update(GameState *game, float dt);

int game_get_wpm(const GameState *game);
int game_get_accuracy(const GameState *game);

void game_load_high_score(GameState *game);
void game_save_high_score(const GameState *game);

void game_save(const GameState *game);
bool game_load(GameState *game);
void game_delete_save(void);

// Word Bank loader
int game_load_words(void);
const char *game_get_random_word(int min_len, int max_len);

#endif /* GAME_TYPING_H */
