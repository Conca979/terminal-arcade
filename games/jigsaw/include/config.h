#ifndef JIGSAW_CONFIG_H
#define JIGSAW_CONFIG_H

#include <stdbool.h>

#define GRID_SIZE 4
#define TOTAL_PIECES (GRID_SIZE * GRID_SIZE)

#define TILE_WIDTH 8
#define TILE_HEIGHT 3

#define ART_WIDTH (GRID_SIZE * TILE_WIDTH)
#define ART_HEIGHT (GRID_SIZE * TILE_HEIGHT)

#define THEME_COUNT 5

#define INNER_WIDTH 92
#define VIEWPORT_LINES 25

typedef enum {
  FOCUS_BOARD = 0,
  FOCUS_TRAY = 1
} FocusArea;

typedef enum {
  ACT_NONE = 0,
  ACT_UP,
  ACT_DOWN,
  ACT_LEFT,
  ACT_RIGHT,
  ACT_SELECT,
  ACT_RETURN,
  ACT_TAB,
  ACT_HINT,
  ACT_THEME,
  ACT_PREVIEW,
  ACT_UNDO,
  ACT_RESTART,
  ACT_QUIT,
  ACT_MOUSE_CLICK,
  ACT_MOUSE_RIGHT
} GameAction;

typedef enum {
  THEME_SUNSET = 0,
  THEME_CASTLE = 1,
  THEME_SPACE = 2,
  THEME_CAT = 3,
  THEME_NUMBERS = 4
} ThemeId;

typedef struct {
  char fg_color[32];
  char bg_color[32];
  char glyph[8];
} PixelCell;

typedef struct {
  const char *title;
  const char *category;
  PixelCell canvas[ART_HEIGHT][ART_WIDTH];
} JigsawTheme;

extern JigsawTheme JIGSAW_THEMES[THEME_COUNT];
const JigsawTheme *jigsaw_get_theme(int theme_idx);

#endif /* JIGSAW_CONFIG_H */
