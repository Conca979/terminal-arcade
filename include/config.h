#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>

#define BOARD_WIDTH 10
#define BOARD_HEIGHT 20
#define BUFFER_HEIGHT 2
#define TOTAL_HEIGHT (BOARD_HEIGHT + BUFFER_HEIGHT)

#define PREVIEW_COUNT 3
#define LOCK_DELAY_MS 500
#define MAX_LOCK_MOVES 15
#define LINE_CLEAR_DELAY_MS 160
#define HIGH_SCORE_FILE "tetris_highscore.txt"

typedef enum {
  TET_NONE = 0,
  TET_I = 1,
  TET_J = 2,
  TET_L = 3,
  TET_O = 4,
  TET_S = 5,
  TET_T = 6,
  TET_Z = 7,
  TET_COUNT = 8
} TetrominoType;

typedef enum {
  STATE_START,
  STATE_PLAYING,
  STATE_PAUSED,
  STATE_LINE_CLEAR,
  STATE_GAME_OVER,
  STATE_EXIT
} PlayState;

typedef enum {
  ACT_NONE = 0,
  ACT_LEFT,
  ACT_RIGHT,
  ACT_DOWN,
  ACT_HARD_DROP,
  ACT_ROTATE_CW,
  ACT_ROTATE_CCW,
  ACT_HOLD,
  ACT_PAUSE,
  ACT_RESTART,
  ACT_QUIT
} GameAction;

#endif /* CONFIG_H */
