#ifndef LIFE_CONFIG_H
#define LIFE_CONFIG_H

#include <stdbool.h>

#define GRID_COLS 30
#define GRID_ROWS 18
#define TOTAL_CELLS (GRID_COLS * GRID_ROWS)

#define CELL_CHAR_WIDTH 2
#define ARENA_WIDTH (GRID_COLS * CELL_CHAR_WIDTH)

#define LEFT_PANEL_WIDTH (ARENA_WIDTH + 2) /* 62 columns */
#define RIGHT_PANEL_WIDTH 29
#define INNER_WIDTH (LEFT_PANEL_WIDTH + 1 + RIGHT_PANEL_WIDTH) /* 92 columns */
#define VIEWPORT_LINES 26

#define MAX_PRESETS 10
#define MAX_HISTORY 128

typedef enum {
  WRAP_TOROIDAL = 0,
  WRAP_BOUNDED = 1
} WrapMode;

typedef enum {
  ACT_NONE = 0,
  ACT_UP,
  ACT_DOWN,
  ACT_LEFT,
  ACT_RIGHT,
  ACT_TOGGLE_CELL,
  ACT_PLAY_PAUSE,
  ACT_STEP_FORWARD,
  ACT_STEP_BACK,
  ACT_CLEAR,
  ACT_RANDOMIZE,
  ACT_PRESET_CYCLE,
  ACT_STAMP_PRESET,
  ACT_WRAP_TOGGLE,
  ACT_SPEED_UP,
  ACT_SPEED_DOWN,
  ACT_MOUSE_CLICK,
  ACT_MOUSE_RIGHT,
  ACT_QUIT
} GameAction;

typedef struct {
  const char *name;
  const char *category;
  int width;
  int height;
  const char *pattern[14]; /* Max 14 rows of 'O' and '.' */
} LifePreset;

extern const LifePreset LIFE_PRESETS[MAX_PRESETS];

#endif /* LIFE_CONFIG_H */
