#ifndef CONFIG_FLAPPY_H
#define CONFIG_FLAPPY_H

#include <stdbool.h>

#define PLAYFIELD_WIDTH 68
#define SKY_HEIGHT 15
#define GROUND_HEIGHT 2
#define TOTAL_HEIGHT (SKY_HEIGHT + GROUND_HEIGHT)

#define BIRD_X 14.0f
#define BIRD_WIDTH 3
#define BIRD_START_Y 7.0f

#define GRAVITY 28.0f
#define FLAP_IMPULSE -10.0f
#define MAX_FALL_SPEED 16.0f
#define MAX_RISE_SPEED -12.0f

#define PIPE_WIDTH 5
#define PIPE_GAP 5
#define PIPE_SPACING 23.0f
#define PIPE_SPEED 14.0f
#define MAX_PIPES 4

#define MEDAL_BRONZE 10
#define MEDAL_SILVER 20
#define MEDAL_GOLD 30
#define MEDAL_PLATINUM 40

typedef enum {
  MEDAL_NONE = 0,
  MEDAL_BRONZE_TIER,
  MEDAL_SILVER_TIER,
  MEDAL_GOLD_TIER,
  MEDAL_PLATINUM_TIER
} MedalTier;

typedef enum {
  STATE_READY,     // Floating gently waiting for first flap
  STATE_PLAYING,   // Active flight & pipes scrolling
  STATE_GAME_OVER, // Crashed into pipe or ground
  STATE_PAUSED,    // Paused
  STATE_EXIT       // Exit to arcade hub
} PlayState;

typedef enum {
  ACT_NONE = 0,
  ACT_FLAP,
  ACT_PAUSE,
  ACT_RESTART,
  ACT_QUIT
} GameAction;

#endif /* CONFIG_FLAPPY_H */
