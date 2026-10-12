#include "render.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

#define COLOR_RESET "\033[0m"
#define COLOR_BORDER "\033[1;38;2;80;120;180m"
#define COLOR_TITLE "\033[1;38;2;255;255;255m"
#define COLOR_LABEL "\033[38;2;140;160;190m"
#define COLOR_VALUE "\033[1;38;2;255;215;60m"
#define COLOR_ACCENT "\033[1;38;2;0;220;210m"
#define COLOR_BANNER "\033[1;38;2;255;230;80;48;2;20;30;50m"

#define COLOR_NORMAL "\033[1;38;2;160;210;255m"
#define COLOR_FAST "\033[1;38;2;255;100;70m"
#define COLOR_TANK "\033[1;38;2;230;100;255m"
#define COLOR_EMP "\033[1;38;2;255;230;50m"

#define COLOR_TYPED "\033[1;38;2;50;255;120m"
#define COLOR_REMAINING "\033[1;38;2;255;255;255m"
#define COLOR_RETICLE "\033[1;38;2;255;220;0m"
#define COLOR_LASER "\033[1;38;2;0;255;255m"
#define COLOR_EXPLOSION "\033[1;38;2;255;180;50m"

#define COLOR_SHIELD_OK "\033[1;38;2;50;230;120m"
#define COLOR_SHIELD_DMG "\033[1;38;2;255;60;60m"

void render_init(void) {
  printf("\033[2J\033[H");
  fflush(stdout);
}

void render_cleanup(void) {
  printf(COLOR_RESET);
  fflush(stdout);
}

void render_frame(const GameState *game) {
  static char buffer[65536];
  int offset = 0;

  offset += snprintf(buffer + offset, sizeof(buffer) - offset, "\033[H");

  // Title Banner
  offset += snprintf(
    buffer + offset, sizeof(buffer) - offset,
    "  %s╔══════════════════════════════════════════════════════════════════════════════╗%s\n"
    "  %s║%s                      %sT Y P I N G   D E F E N S E   ( C )%s                   %s║%s\n"
    "  %s╚══════════════════════════════════════════════════════════════════════════════╝%s\n",
    COLOR_BORDER, COLOR_RESET,
    COLOR_BORDER, COLOR_RESET, COLOR_TITLE, COLOR_RESET, COLOR_BORDER, COLOR_RESET,
    COLOR_BORDER, COLOR_RESET
  );

  // Status Bar
  int wpm = game_get_wpm(game);
  int acc = game_get_accuracy(game);

  char shield_bar[32] = "";
  for (int i = 0; i < MAX_SHIELD_HP; i++) {
    if (i < game->shield_hp) strcat(shield_bar, "█");
    else strcat(shield_bar, "░");
  }

  const char *shield_col = (game->shield_hp <= 1 || game->shield_flash_ms > 0) ? COLOR_SHIELD_DMG : COLOR_SHIELD_OK;

  offset += snprintf(
    buffer + offset, sizeof(buffer) - offset,
    "  %sSCORE:%s %s%06d%s  %sWAVE:%s %s%02d%s  %sSHIELDS:%s %s[%s] %d%%%s  %sWPM:%s %s%02d%s  %sACC:%s %s%3d%%%s  %sCOMBO:%s %sx%d%s\n",
    COLOR_LABEL, COLOR_RESET, COLOR_VALUE, game->score, COLOR_RESET,
    COLOR_LABEL, COLOR_RESET, COLOR_ACCENT, game->wave, COLOR_RESET,
    COLOR_LABEL, COLOR_RESET, shield_col, shield_bar, (game->shield_hp * 20), COLOR_RESET,
    COLOR_LABEL, COLOR_RESET, COLOR_VALUE, wpm, COLOR_RESET,
    COLOR_LABEL, COLOR_RESET, COLOR_VALUE, acc, COLOR_RESET,
    COLOR_LABEL, COLOR_RESET, COLOR_ACCENT, game->current_combo, COLOR_RESET
  );

  // Screen Grid representation for arena
  // Rows 4 to 21 (height 18)
  static char arena[ARENA_HEIGHT][ARENA_WIDTH + 1];
  static int color_grid[ARENA_HEIGHT][ARENA_WIDTH];
  // color_grid codes:
  // 0 = empty space
  // 1 = normal word, 2 = fast, 3 = tank, 4 = emp
  // 5 = typed char, 6 = remaining char, 7 = reticle
  // 8 = laser beam, 9 = explosion
  for (int r = 0; r < ARENA_HEIGHT; r++) {
    memset(arena[r], ' ', ARENA_WIDTH);
    arena[r][ARENA_WIDTH] = '\0';
    memset(color_grid[r], 0, sizeof(color_grid[r]));
  }

  // Draw Laser Beams
  for (int i = 0; i < MAX_LASERS; i++) {
    if (game->lasers[i].active) {
      int tx = (int)roundf(game->lasers[i].target_x);
      int ty = (int)roundf(game->lasers[i].target_y);
      int sx = ARENA_WIDTH / 2;
      int sy = PLAYABLE_BOTTOM;

      int steps = sy - ty;
      if (steps > 0) {
        for (int y = ty; y <= sy; y++) {
          float t = (float)(y - ty) / (float)steps;
          int x = (int)roundf((float)tx + t * (float)(sx - tx));
          if (y >= PLAYABLE_TOP && y < ARENA_HEIGHT && x >= 0 && x < ARENA_WIDTH) {
            arena[y][x] = '|';
            color_grid[y][x] = 8;
          }
        }
      }
    }
  }

  // Draw Explosions
  for (int i = 0; i < MAX_EXPLOSIONS; i++) {
    if (game->explosions[i].active) {
      int ex = (int)roundf(game->explosions[i].x);
      int ey = (int)roundf(game->explosions[i].y);
      const char *pattern = (game->explosions[i].frame < 4) ? "* ✦ *" : "· · ·";
      int plen = (int)strlen(pattern);
      int sx = ex - plen / 2;
      for (int c = 0; c < plen; c++) {
        int x = sx + c;
        if (ey >= PLAYABLE_TOP && ey < ARENA_HEIGHT && x >= 0 && x < ARENA_WIDTH) {
          if (pattern[c] != ' ') {
            arena[ey][x] = pattern[c];
            color_grid[ey][x] = 9;
          }
        }
      }
    }
  }

  // Draw Enemies and their words
  for (int i = 0; i < MAX_ACTIVE_ENEMIES; i++) {
    if (game->enemies[i].active) {
      const Enemy *e = &game->enemies[i];
      int ey = (int)roundf(e->y);
      int ex = (int)roundf(e->x);

      if (ey >= PLAYABLE_TOP && ey < PLAYABLE_BOTTOM && ex >= 1 && ex + e->word_len + 4 < ARENA_WIDTH) {
        bool is_locked = (game->target_enemy_idx == i);

        // Reticle / Badge prefix
        const char *badge = "▼";
        int btype = 1;
        if (e->type == ENEMY_FAST) { badge = "▲"; btype = 2; }
        else if (e->type == ENEMY_TANK) { badge = "♦"; btype = 3; }
        else if (e->type == ENEMY_EMP) { badge = "⚡"; btype = 4; }

        if (is_locked) {
          arena[ey][ex] = '>';
          color_grid[ey][ex] = 7;
        } else {
          arena[ey][ex] = badge[0];
          color_grid[ey][ex] = btype;
        }

        // Draw Word letters
        for (int w = 0; w < e->word_len; w++) {
          int x = ex + 2 + w;
          if (x < ARENA_WIDTH) {
            arena[ey][x] = e->word[w];
            if (is_locked) {
              color_grid[ey][x] = (w < e->typed_len) ? 5 : 6;
            } else {
              color_grid[ey][x] = btype;
            }
          }
        }

        if (is_locked) {
          int rx = ex + 2 + e->word_len;
          if (rx < ARENA_WIDTH) {
            arena[ey][rx] = '<';
            color_grid[ey][rx] = 7;
          }
        }
      }
    }
  }

  // Print Arena Box Top
  offset += snprintf(buffer + offset, sizeof(buffer) - offset, "  %s┌", COLOR_BORDER);
  for (int c = 0; c < ARENA_WIDTH; c++) {
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "─");
  }
  offset += snprintf(buffer + offset, sizeof(buffer) - offset, "┐%s\n", COLOR_RESET);

  // Print Arena lines (PLAYABLE_TOP to PLAYABLE_BOTTOM - 1)
  for (int r = PLAYABLE_TOP; r < PLAYABLE_BOTTOM; r++) {
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "  %s│%s", COLOR_BORDER, COLOR_RESET);

    for (int c = 0; c < ARENA_WIDTH; c++) {
      char ch = arena[r][c];
      int code = color_grid[r][c];

      switch (code) {
        case 1: // Normal enemy
          offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s%c%s", COLOR_NORMAL, ch, COLOR_RESET);
          break;
        case 2: // Fast enemy
          offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s%c%s", COLOR_FAST, ch, COLOR_RESET);
          break;
        case 3: // Tank enemy
          offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s%c%s", COLOR_TANK, ch, COLOR_RESET);
          break;
        case 4: // EMP enemy
          offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s%c%s", COLOR_EMP, ch, COLOR_RESET);
          break;
        case 5: // Typed letters
          offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s%c%s", COLOR_TYPED, ch, COLOR_RESET);
          break;
        case 6: // Remaining letters of targeted word
          offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s%c%s", COLOR_REMAINING, ch, COLOR_RESET);
          break;
        case 7: // Target reticle brackets
          offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s%c%s", COLOR_RETICLE, ch, COLOR_RESET);
          break;
        case 8: // Laser beam
          offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s%c%s", COLOR_LASER, ch, COLOR_RESET);
          break;
        case 9: // Explosion
          offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s%c%s", COLOR_EXPLOSION, ch, COLOR_RESET);
          break;
        default:
          buffer[offset++] = ch;
          break;
      }
    }

    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s│%s\n", COLOR_BORDER, COLOR_RESET);
  }

  // Base Perimeter / Citadel Cannons Line
  offset += snprintf(
    buffer + offset, sizeof(buffer) - offset,
    "  %s╞═══════════════════════════[ ⚡ CITADEL CANNON ⚡ ]═══════════════════════════╡%s\n",
    (game->shield_flash_ms > 0) ? COLOR_SHIELD_DMG : COLOR_BORDER, COLOR_RESET
  );

  // Bottom Interactive Target Bar
  if (game->state == STATE_WAVE_INTRO) {
    offset += snprintf(
      buffer + offset, sizeof(buffer) - offset,
      "  %s          >>> INCOMING ATTACK WAVE %02d - GET READY TO TYPE! <<<          %s\n",
      COLOR_BANNER, game->wave, COLOR_RESET
    );
  } else if (game->state == STATE_PAUSED) {
    offset += snprintf(
      buffer + offset, sizeof(buffer) - offset,
      "  %s               >>> MISSION PAUSED - PRESS P TO RESUME <<<                 %s\n",
      COLOR_BANNER, COLOR_RESET
    );
  } else if (game->state == STATE_GAME_OVER) {
    offset += snprintf(
      buffer + offset, sizeof(buffer) - offset,
      "  %s    ★ CITADEL BREACHED - GAME OVER! PRESS [ENTER / SPACE] TO RETRY ★      %s\n",
      COLOR_BANNER, COLOR_RESET
    );
  } else if (game->target_enemy_idx != -1) {
    const Enemy *target = &game->enemies[game->target_enemy_idx];
    char typed_buf[MAX_WORD_LEN] = "";
    strncpy(typed_buf, target->word, target->typed_len);
    typed_buf[target->typed_len] = '\0';

    offset += snprintf(
      buffer + offset, sizeof(buffer) - offset,
      "  %sLOCKED:%s %s[ %s ]%s   %sPROGRESS:%s %s%s%s%s%s   %s[Backspace]%s Cancel Target\n",
      COLOR_LABEL, COLOR_RESET, COLOR_VALUE, target->word, COLOR_RESET,
      COLOR_LABEL, COLOR_RESET,
      COLOR_TYPED, typed_buf, COLOR_RESET,
      COLOR_REMAINING, target->word + target->typed_len,
      COLOR_LABEL, COLOR_RESET
    );
  } else {
    offset += snprintf(
      buffer + offset, sizeof(buffer) - offset,
      "  %sCITADEL STATUS: READY%s   %sType first letter of any incoming word to target%s\n",
      COLOR_ACCENT, COLOR_RESET, COLOR_LABEL, COLOR_RESET
    );
  }

  // Footer Controls Reference
  offset += snprintf(
    buffer + offset, sizeof(buffer) - offset,
    "\n  %sControls:%s %s[A - Z]%s Type Words  │  %s[Backspace]%s Unlock Target  │  %s[P]%s Pause  │  %s[R]%s Restart  │  %s[Q / Esc]%s Quit\n",
    COLOR_LABEL, COLOR_RESET,
    COLOR_TITLE, COLOR_RESET,
    COLOR_TITLE, COLOR_RESET,
    COLOR_TITLE, COLOR_RESET,
    COLOR_TITLE, COLOR_RESET,
    COLOR_TITLE, COLOR_RESET
  );

  fwrite(buffer, 1, offset, stdout);
  fflush(stdout);
}
