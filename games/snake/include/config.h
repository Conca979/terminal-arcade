#ifndef CONFIG_SNAKE_H
#define CONFIG_SNAKE_H

#include <stdbool.h>

#define ARENA_WIDTH 32
#define ARENA_HEIGHT 18
#define MAX_SNAKE_LEN (ARENA_WIDTH * ARENA_HEIGHT)
#define HIGH_SCORE_FILE "snake_highscore.txt"

#define BASE_TICK_MS 110
#define MIN_TICK_MS 45
#define BONUS_DURATION_MS 9000
#define BUFF_DURATION_MS 7000

typedef enum {
  DIR_UP = 0,
  DIR_DOWN,
  DIR_LEFT,
  DIR_RIGHT
} Direction;

typedef enum {
  FOOD_NONE = 0,
  FOOD_DATA,       // Regular data packet (+10 pts)
  FOOD_CORE,       // Bonus cyber core (+50 pts, timed)
  FOOD_OVERCLOCK,  // 2x score multiplier buff
  FOOD_PHASE       // Phase through walls buff
} FoodType;

typedef enum {
  STATE_READY,
  STATE_PLAYING,
  STATE_PAUSED,
  STATE_GAME_OVER,
  STATE_EXIT
} PlayState;

typedef enum {
  ACT_NONE = 0,
  ACT_UP,
  ACT_DOWN,
  ACT_LEFT,
  ACT_RIGHT,
  ACT_PAUSE,
  ACT_TOGGLE_WRAP,
  ACT_RESTART,
  ACT_QUIT
} GameAction;

#endif /* CONFIG_SNAKE_H */
