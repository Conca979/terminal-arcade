#include "render.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

#define COLOR_RESET        "\033[0m"
#define COLOR_BORDER       "\033[38;2;70;110;165m"
#define COLOR_TITLE        "\033[1;38;2;255;255;255m"
#define COLOR_LABEL        "\033[38;2;140;160;190m"
#define COLOR_VAL          "\033[1;38;2;255;215;60m"
#define COLOR_ACCENT       "\033[1;38;2;0;225;215m"

#define COLOR_SKY_BG       "\033[48;2;12;22;38m"
#define COLOR_CLOUD        "\033[38;2;165;195;225;48;2;12;22;38m"

#define COLOR_PIPE_LIP     "\033[1;38;2;85;240;120;48;2;25;110;45m"
#define COLOR_PIPE_BODY    "\033[1;38;2;40;190;75;48;2;18;85;35m"

#define COLOR_BIRD_BODY    "\033[1;38;2;255;225;50;48;2;12;22;38m"
#define COLOR_BIRD_WING    "\033[1;38;2;255;240;120;48;2;12;22;38m"
#define COLOR_BIRD_BEAK    "\033[1;38;2;255;130;20;48;2;12;22;38m"
#define COLOR_BIRD_DEAD    "\033[1;38;2;240;75;75;48;2;12;22;38m"

#define COLOR_GRASS        "\033[1;38;2;46;204;113;48;2;20;120;50m"
#define COLOR_DIRT         "\033[38;2;190;140;80;48;2;85;55;25m"
#define COLOR_DIRT_ALT     "\033[38;2;160;110;60;48;2;75;45;20m"

#define COLOR_MODAL_BG     "\033[1;38;2;255;255;255;48;2;30;45;70m"
#define COLOR_MODAL_BORDER "\033[1;38;2;0;225;215;48;2;30;45;70m"
#define COLOR_MODAL_TXT    "\033[1;38;2;255;255;255;48;2;30;45;70m"
#define COLOR_MODAL_VAL    "\033[1;38;2;255;215;60;48;2;30;45;70m"
#define COLOR_MODAL_DIM    "\033[38;2;170;195;230;48;2;30;45;70m"

typedef struct {
  char ch;
  const char *col;
} Cell;

static Cell grid[SKY_HEIGHT][PLAYFIELD_WIDTH];

void render_init(void) {
  printf("\033[2J\033[H");
  fflush(stdout);
}

void render_cleanup(void) {
  printf(COLOR_RESET);
  fflush(stdout);
}

static const char *get_medal_str(MedalTier medal) {
  switch (medal) {
    case MEDAL_PLATINUM_TIER: return "PLATINUM";
    case MEDAL_GOLD_TIER:     return "GOLD";
    case MEDAL_SILVER_TIER:   return "SILVER";
    case MEDAL_BRONZE_TIER:   return "BRONZE";
    default:                  return "NONE";
  }
}

static void stamp_string(int y, int x, const char *str, const char *col) {
  if (y < 0 || y >= SKY_HEIGHT) return;
  for (int i = 0; str[i] != '\0'; i++) {
    int cx = x + i;
    if (cx >= 0 && cx < PLAYFIELD_WIDTH) {
      grid[y][cx].ch = str[i];
      grid[y][cx].col = col;
    }
  }
}

void render_frame(const GameState *game) {
  static char buffer[32768];
  int offset = 0;

  // Clear grid cells to default empty sky
  for (int y = 0; y < SKY_HEIGHT; y++) {
    for (int x = 0; x < PLAYFIELD_WIDTH; x++) {
      grid[y][x].ch = ' ';
      grid[y][x].col = COLOR_SKY_BG;
    }
  }

  // 1. Draw Parallax Clouds
  for (int c = 0; c < MAX_CLOUDS; c++) {
    int cy = (int)roundf(game->clouds[c].y);
    int cx = (int)roundf(game->clouds[c].x);
    const char *cloud_art = (game->clouds[c].type == 0) ? "(   )" :
                            (game->clouds[c].type == 1) ? "(     )" : "(  ~  )";
    stamp_string(cy, cx, cloud_art, COLOR_CLOUD);
  }

  // 2. Draw Pipes
  for (int p = 0; p < MAX_PIPES; p++) {
    int px = (int)roundf(game->pipes[p].x);
    int gap_top = game->pipes[p].gap_y;
    int gap_bot = gap_top + PIPE_GAP;

    for (int off = 0; off < PIPE_WIDTH; off++) {
      int cx = px + off;
      if (cx < 0 || cx >= PLAYFIELD_WIDTH) continue;

      for (int y = 0; y < SKY_HEIGHT; y++) {
        if (y < gap_top) {
          // Top pipe
          if (y == gap_top - 1) {
            char cap = (off == 0) ? '[' : (off == PIPE_WIDTH - 1) ? ']' : '=';
            grid[y][cx].ch = cap;
            grid[y][cx].col = COLOR_PIPE_LIP;
          } else {
            char body = (off == 0 || off == PIPE_WIDTH - 1) ? '|' : (off == 1) ? '/' : ' ';
            grid[y][cx].ch = body;
            grid[y][cx].col = COLOR_PIPE_BODY;
          }
        } else if (y >= gap_bot) {
          // Bottom pipe
          if (y == gap_bot) {
            char cap = (off == 0) ? '[' : (off == PIPE_WIDTH - 1) ? ']' : '=';
            grid[y][cx].ch = cap;
            grid[y][cx].col = COLOR_PIPE_LIP;
          } else {
            char body = (off == 0 || off == PIPE_WIDTH - 1) ? '|' : (off == 1) ? '/' : ' ';
            grid[y][cx].ch = body;
            grid[y][cx].col = COLOR_PIPE_BODY;
          }
        }
      }
    }
  }

  // 3. Draw Bird
  int by = (int)roundf(game->bird.y);
  int bx = (int)BIRD_X;
  if (by >= 0 && by < SKY_HEIGHT) {
    if (game->state == STATE_GAME_OVER) {
      if (bx >= 0 && bx < PLAYFIELD_WIDTH) { grid[by][bx].ch = 'x'; grid[by][bx].col = COLOR_BIRD_DEAD; }
      if (bx + 1 < PLAYFIELD_WIDTH) { grid[by][bx + 1].ch = '('; grid[by][bx + 1].col = COLOR_BIRD_DEAD; }
      if (bx + 2 < PLAYFIELD_WIDTH) { grid[by][bx + 2].ch = 'x'; grid[by][bx + 2].col = COLOR_BIRD_DEAD; }
      if (bx + 3 < PLAYFIELD_WIDTH) { grid[by][bx + 3].ch = '>'; grid[by][bx + 3].col = COLOR_BIRD_BEAK; }
    } else {
      char wing = (game->bird.vy < -2.0f || game->bird.flap_anim_timer > 0.0f) ? '\\' :
                  (game->bird.vy > 3.0f) ? '/' : '-';
      if (bx >= 0 && bx < PLAYFIELD_WIDTH) { grid[by][bx].ch = wing; grid[by][bx].col = COLOR_BIRD_WING; }
      if (bx + 1 < PLAYFIELD_WIDTH) { grid[by][bx + 1].ch = '('; grid[by][bx + 1].col = COLOR_BIRD_BODY; }
      if (bx + 2 < PLAYFIELD_WIDTH) { grid[by][bx + 2].ch = 'o'; grid[by][bx + 2].col = COLOR_BIRD_BODY; }
      if (bx + 3 < PLAYFIELD_WIDTH) { grid[by][bx + 3].ch = '>'; grid[by][bx + 3].col = COLOR_BIRD_BEAK; }
    }
  }

  // 4. Center Modals / Overlays (Exact 40 columns wide, centered at x = 14)
  if (game->state == STATE_READY) {
    stamp_string(5, 14, "+======================================+", COLOR_MODAL_BORDER);
    stamp_string(6, 14, "|   * GET READY! TAP [SPACE] TO FLAP * |", COLOR_MODAL_TXT);
    stamp_string(7, 14, "+======================================+", COLOR_MODAL_BORDER);
  } else if (game->state == STATE_PAUSED) {
    stamp_string(5, 14, "+======================================+", COLOR_MODAL_BORDER);
    stamp_string(6, 14, "|           || GAME PAUSED ||          |", COLOR_MODAL_TXT);
    stamp_string(7, 14, "|    PRESS [P] OR [SPACE] TO RESUME    |", COLOR_MODAL_DIM);
    stamp_string(8, 14, "+======================================+", COLOR_MODAL_BORDER);
  } else if (game->state == STATE_GAME_OVER) {
    char row1[48], row2[48];
    snprintf(row1, sizeof(row1), "|  FINAL SCORE: %-3d      BEST: %-3d    |", game->score, game->high_score);
    snprintf(row2, sizeof(row2), "|  MEDAL: %-10s   (REPLAY: SPACE) |", get_medal_str(game->medal));

    stamp_string(4, 14, "+============== GAME OVER =============+", COLOR_MODAL_BORDER);
    stamp_string(5, 14, row1, COLOR_MODAL_TXT);
    stamp_string(6, 14, row2, COLOR_MODAL_VAL);
    stamp_string(7, 14, "+======================================+", COLOR_MODAL_BORDER);
  }

  // --- OUTPUT GENERATION (Exactly 21 Lines) ---
  offset += snprintf(buffer + offset, sizeof(buffer) - offset, "\033[H");

  // Line 1: Top Border
  offset += snprintf(
    buffer + offset, sizeof(buffer) - offset,
    "  %s╔════════════════════════════════════════════════════════════════════╗%s\n",
    COLOR_BORDER, COLOR_RESET
  );

  // Line 2: HUD Bar (Exactly 68 columns inside border)
  const char *medal_name = get_medal_str(game->medal);

  offset += snprintf(
    buffer + offset, sizeof(buffer) - offset,
    "  %s║%s %sSCORE:%s %s%-3d%s     %sBEST:%s %s%-3d%s     %sMEDAL:%s %s%-8s%s     %sFLAPPY BIRD (C)%s   %s║%s\n",
    COLOR_BORDER, COLOR_RESET,
    COLOR_LABEL, COLOR_RESET, COLOR_VAL, game->score, COLOR_RESET,
    COLOR_LABEL, COLOR_RESET, COLOR_VAL, game->high_score, COLOR_RESET,
    COLOR_LABEL, COLOR_RESET, COLOR_ACCENT, medal_name, COLOR_RESET,
    COLOR_TITLE, COLOR_RESET,
    COLOR_BORDER, COLOR_RESET
  );

  // Lines 3 to 17: Sky Rows (15 rows)
  for (int y = 0; y < SKY_HEIGHT; y++) {
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "  %s║%s", COLOR_BORDER, COLOR_RESET);

    const char *last_col = "";
    for (int x = 0; x < PLAYFIELD_WIDTH; x++) {
      if (grid[y][x].col != last_col) {
        last_col = grid[y][x].col;
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s", last_col);
      }
      buffer[offset++] = grid[y][x].ch;
    }

    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s%s║%s\n", COLOR_RESET, COLOR_BORDER, COLOR_RESET);
  }

  // Line 18: Grass Row
  offset += snprintf(buffer + offset, sizeof(buffer) - offset, "  %s║%s%s", COLOR_BORDER, COLOR_RESET, COLOR_GRASS);
  for (int x = 0; x < PLAYFIELD_WIDTH; x++) {
    buffer[offset++] = '=';
  }
  offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s%s║%s\n", COLOR_RESET, COLOR_BORDER, COLOR_RESET);

  // Line 19: Dirt Row (Scrolling texture)
  int dirt_shift = (int)game->ground_offset % 4;
  offset += snprintf(buffer + offset, sizeof(buffer) - offset, "  %s║%s", COLOR_BORDER, COLOR_RESET);
  for (int x = 0; x < PLAYFIELD_WIDTH; x++) {
    int pat = (x + dirt_shift) % 4;
    const char *dcol = (pat < 2) ? COLOR_DIRT : COLOR_DIRT_ALT;
    char dch = (pat == 0) ? '#' : (pat == 1) ? '%' : (pat == 2) ? ':' : '.';
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s%c", dcol, dch);
  }
  offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s%s║%s\n", COLOR_RESET, COLOR_BORDER, COLOR_RESET);

  // Line 20: Bottom Border
  offset += snprintf(
    buffer + offset, sizeof(buffer) - offset,
    "  %s╚════════════════════════════════════════════════════════════════════╝%s\n",
    COLOR_BORDER, COLOR_RESET
  );

  // Line 21: Controls Reference
  if (game->state == STATE_GAME_OVER) {
    offset += snprintf(
      buffer + offset, sizeof(buffer) - offset,
      "  %sControls:%s %s[Space / Enter / R]%s Play Again  │  %s[Q / Esc]%s Return to Arcade\n",
      COLOR_LABEL, COLOR_RESET, COLOR_TITLE, COLOR_RESET, COLOR_TITLE, COLOR_RESET
    );
  } else {
    offset += snprintf(
      buffer + offset, sizeof(buffer) - offset,
      "  %sControls:%s %s[Space / W / ↑]%s Flap  │  %s[P]%s Pause  │  %s[R]%s Restart  │  %s[Q / Esc]%s Quit\n",
      COLOR_LABEL, COLOR_RESET, COLOR_TITLE, COLOR_RESET, COLOR_TITLE, COLOR_RESET, COLOR_TITLE, COLOR_RESET, COLOR_TITLE, COLOR_RESET
    );
  }

  fwrite(buffer, 1, offset, stdout);
  fflush(stdout);
}
