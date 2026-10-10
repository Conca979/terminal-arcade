#ifndef CONFIG_2048_H
#define CONFIG_2048_H

#include <stdbool.h>

#define GRID_SIZE 4
#define MAX_UNDO_STATES 32
#define HIGH_SCORE_FILE "2048_highscore.txt"
#define WIN_TARGET 2048

typedef enum {
  DIR_NONE = 0,
  DIR_UP,
  DIR_DOWN,
  DIR_LEFT,
  DIR_RIGHT
} MoveDirection;

typedef enum {
  ACT_NONE = 0,
  ACT_MOVE_UP,
  ACT_MOVE_DOWN,
  ACT_MOVE_LEFT,
  ACT_MOVE_RIGHT,
  ACT_UNDO,
  ACT_RESTART,
  ACT_CONTINUE,
  ACT_QUIT
} GameAction;

typedef enum {
  STATE_PLAYING,
  STATE_WON,
  STATE_GAME_OVER,
  STATE_EXIT
} PlayState;

#endif /* CONFIG_2048_H */
