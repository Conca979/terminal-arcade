#ifndef CONFIG_MINESWEEPER_H
#define CONFIG_MINESWEEPER_H

#include <stdbool.h>

#define MAX_ROWS 16
#define MAX_COLS 30
#define MAX_MINES 99
#define SCORES_FILE "minesweeper_scores.txt"

typedef enum {
  DIFF_BEGINNER = 0,     // 9x9, 10 mines
  DIFF_INTERMEDIATE = 1, // 16x16, 40 mines
  DIFF_EXPERT = 2        // 30x16, 99 mines
} Difficulty;

typedef enum {
  STATE_READY,
  STATE_PLAYING,
  STATE_WON,
  STATE_GAME_OVER,
  STATE_EXIT
} PlayState;

typedef enum {
  ACT_NONE = 0,
  ACT_UP,
  ACT_DOWN,
  ACT_LEFT,
  ACT_RIGHT,
  ACT_REVEAL,
  ACT_FLAG,
  ACT_CHORD,
  ACT_DIFF_1,
  ACT_DIFF_2,
  ACT_DIFF_3,
  ACT_RESTART,
  ACT_QUIT
} GameAction;

#endif /* CONFIG_MINESWEEPER_H */
