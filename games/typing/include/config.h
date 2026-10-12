#ifndef CONFIG_TYPING_H
#define CONFIG_TYPING_H

#include <stdbool.h>

#define ARENA_WIDTH 80
#define ARENA_HEIGHT 24
#define PLAYABLE_TOP 4
#define PLAYABLE_BOTTOM 21

#define MAX_ACTIVE_ENEMIES 12
#define MAX_WORD_LEN 24
#define MAX_EXPLOSIONS 8
#define MAX_LASERS 4
#define MAX_SHIELD_HP 5
#define MAX_BANK_WORDS 6000

#define WORDS_PER_WAVE 10
#define BASE_DESCENT_SPEED 0.65f
#define SPEED_INCREASE_PER_WAVE 0.12f

typedef enum {
  ENEMY_NORMAL = 0,
  ENEMY_FAST,
  ENEMY_TANK,
  ENEMY_EMP,
  ENEMY_TYPE_COUNT
} EnemyType;

typedef enum {
  STATE_WAVE_INTRO,  // Brief announcement before wave spawns
  STATE_PLAYING,     // Defending citadel
  STATE_PAUSED,      // Mission paused
  STATE_GAME_OVER,   // Shields depleted
  STATE_EXIT         // Returning to Arcade Hub
} PlayState;

typedef enum {
  ACT_NONE = 0,
  ACT_CHAR,
  ACT_BACKSPACE,
  ACT_PAUSE,
  ACT_RESTART,
  ACT_QUIT
} ActionType;

typedef struct {
  ActionType type;
  char ch;
} GameAction;

#endif /* CONFIG_TYPING_H */
