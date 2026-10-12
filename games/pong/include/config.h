#ifndef CONFIG_PONG_H
#define CONFIG_PONG_H

#include <stdbool.h>

#define COURT_WIDTH 76
#define COURT_HEIGHT 22
#define PADDLE_HEIGHT 5
#define PADDLE_HALF 2.0f
#define TRAIL_MAX 5

#define WIN_SCORE 7
#define INITIAL_BALL_SPEED_X 24.0f
#define MAX_BALL_SPEED_X 52.0f
#define SPEED_UP_FACTOR 1.05f
#define PLAYER_PADDLE_SPEED 26.0f

typedef enum {
  DIFF_EASY = 0,
  DIFF_MEDIUM,
  DIFF_HARD,
  DIFF_COUNT
} AiDifficulty;

typedef enum {
  STATE_SERVE,       // Ball waiting to be served
  STATE_PLAYING,     // Live rally
  STATE_POINT_SCORED,// Point scored, brief freeze / flash
  STATE_PAUSED,      // Game paused
  STATE_GAME_OVER,   // Match ended
  STATE_EXIT         // Quit game
} PlayState;

typedef enum {
  ACT_NONE = 0,
  ACT_UP,
  ACT_DOWN,
  ACT_SERVE,
  ACT_TOGGLE_DIFF,
  ACT_PAUSE,
  ACT_RESTART,
  ACT_QUIT
} GameAction;

#endif /* CONFIG_PONG_H */
