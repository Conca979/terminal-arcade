#include "render.h"
#include <stdio.h>
#include <string.h>

#define COLOR_RESET "\033[0m"
#define COLOR_BORDER "\033[38;2;70;110;170m"
#define COLOR_TITLE "\033[1;38;2;255;255;255m"
#define COLOR_LABEL "\033[1;38;2;160;180;210m"
#define COLOR_VALUE "\033[1;38;2;255;220;50m"
#define COLOR_HINT "\033[38;2;140;150;170m"

#define COLOR_UFO "\033[1;38;2;255;70;70m"
#define COLOR_SQUID "\033[1;38;2;230;70;240m"   // Type 0: Magenta
#define COLOR_CRAB "\033[1;38;2;0;220;255m"    // Type 1: Cyan
#define COLOR_OCTOPUS "\033[1;38;2;60;240;120m" // Type 2: Green
#define COLOR_PLAYER "\033[1;38;2;0;255;200m"
#define COLOR_PBULLET "\033[1;38;2;255;255;100m"
#define COLOR_BOMB "\033[1;38;2;255;90;90m"
#define COLOR_EXPLOSION "\033[1;38;2;255;210;60m"

void render_init(void) {
  printf("\033[2J\033[H");
  fflush(stdout);
}

void render_cleanup(void) {
  printf(COLOR_RESET);
  fflush(stdout);
}

void render_frame(const GameState *game) {
  static char buffer[32768];
  int offset = 0;

  offset += snprintf(buffer + offset, sizeof(buffer) - offset, "\033[H");

  // Title Banner
  offset += snprintf(
    buffer + offset, sizeof(buffer) - offset,
    "  %s╔══════════════════════════════════════════════════════════════╗%s\n"
    "  %s║%s             %sS P A C E   I N V A D E R S   ( C )%s            %s║%s\n"
    "  %s╚══════════════════════════════════════════════════════════════╝%s\n\n",
    COLOR_BORDER, COLOR_RESET,
    COLOR_BORDER, COLOR_RESET, COLOR_TITLE, COLOR_RESET, COLOR_BORDER, COLOR_RESET,
    COLOR_BORDER, COLOR_RESET
  );

  // Status Bar: Score, High Score, Wave, Lives
  char lives_str[32] = "";
  for (int i = 0; i < game->player.lives; i++) {
    strcat(lives_str, "▲ ");
  }

  offset += snprintf(
    buffer + offset, sizeof(buffer) - offset,
    "  %sSCORE:%s %s%06d%s   %sHIGH:%s %s%06d%s   %sWAVE:%s %s%02d%s   %sLIVES:%s %s%-8s%s\n\n",
    COLOR_LABEL, COLOR_RESET, COLOR_VALUE, game->score, COLOR_RESET,
    COLOR_LABEL, COLOR_RESET, COLOR_VALUE, game->high_score, COLOR_RESET,
    COLOR_LABEL, COLOR_RESET, COLOR_VALUE, game->wave, COLOR_RESET,
    COLOR_LABEL, COLOR_RESET, COLOR_PLAYER, lives_str, COLOR_RESET
  );

  // Buffer representation for the 48x24 grid
  // Cell display: 0 = empty, 1 = player, 2 = pbullet, 3 = bomb, 4 = squid, 5 = crab, 6 = octopus, 7 = ufo, 8 = bunker, 9 = explosion
  char cell_char[FIELD_HEIGHT][FIELD_WIDTH];
  const char *cell_color[FIELD_HEIGHT][FIELD_WIDTH];

  for (int y = 0; y < FIELD_HEIGHT; y++) {
    for (int x = 0; x < FIELD_WIDTH; x++) {
      cell_char[y][x] = ' ';
      cell_color[y][x] = COLOR_RESET;
    }
  }

  // 1. UFO
  if (game->ufo.active && game->ufo.y >= 0 && game->ufo.y < FIELD_HEIGHT) {
    const char *ufo_str = "<==>";
    for (int i = 0; i < 4; i++) {
      int ux = game->ufo.x + i;
      if (ux >= 0 && ux < FIELD_WIDTH) {
        cell_char[game->ufo.y][ux] = ufo_str[i];
        cell_color[game->ufo.y][ux] = COLOR_UFO;
      }
    }
  }

  // 2. Aliens
  static const char *SQUID_FRAMES[2] = {"/\"\\", "\\\"/"};
  static const char *CRAB_FRAMES[2] = {"|o|", "/o\\"};
  static const char *OCTO_FRAMES[2] = {"/-\\", "\\-/"};

  for (int r = 0; r < ALIEN_ROWS; r++) {
    for (int c = 0; c < ALIEN_COLS; c++) {
      if (!game->aliens[r][c].alive) continue;
      int ax = game->swarm.origin_x + c * 4;
      int ay = game->swarm.origin_y + r * 2;

      const char *sprite = CRAB_FRAMES[game->swarm.anim_frame];
      const char *color = COLOR_CRAB;

      if (game->aliens[r][c].type == 0) {
        sprite = SQUID_FRAMES[game->swarm.anim_frame];
        color = COLOR_SQUID;
      } else if (game->aliens[r][c].type == 2) {
        sprite = OCTO_FRAMES[game->swarm.anim_frame];
        color = COLOR_OCTOPUS;
      }

      if (ay >= 0 && ay < FIELD_HEIGHT) {
        for (int i = 0; i < 3; i++) {
          int px = ax + i;
          if (px >= 0 && px < FIELD_WIDTH) {
            cell_char[ay][px] = sprite[i];
            cell_color[ay][px] = color;
          }
        }
      }
    }
  }

  // 3. Bunkers
  for (int k = 0; k < BUNKER_COUNT; k++) {
    int by = game->bunkers[k].y;
    if (by >= 0 && by < FIELD_HEIGHT) {
      for (int p = 0; p < BUNKER_PARTS; p++) {
        int bx = game->bunkers[k].x + p;
        if (bx >= 0 && bx < FIELD_WIDTH) {
          int hp = game->bunkers[k].hp[p];
          if (hp == 3) {
            cell_char[by][bx] = '#';
            cell_color[by][bx] = "\033[1;38;2;50;220;120m";
          } else if (hp == 2) {
            cell_char[by][bx] = '=';
            cell_color[by][bx] = "\033[38;2;50;180;100m";
          } else if (hp == 1) {
            cell_char[by][bx] = '-';
            cell_color[by][bx] = "\033[38;2;40;130;80m";
          }
        }
      }
    }
  }

  // 4. Player Bullets
  for (int i = 0; i < MAX_PLAYER_BULLETS; i++) {
    if (game->player_bullets[i].active) {
      int bx = game->player_bullets[i].x;
      int by = game->player_bullets[i].y;
      if (bx >= 0 && bx < FIELD_WIDTH && by >= 0 && by < FIELD_HEIGHT) {
        cell_char[by][bx] = '|';
        cell_color[by][bx] = COLOR_PBULLET;
      }
    }
  }

  // 5. Alien Bombs
  for (int i = 0; i < MAX_ALIEN_BOMBS; i++) {
    if (game->alien_bombs[i].active) {
      int bx = game->alien_bombs[i].x;
      int by = game->alien_bombs[i].y;
      if (bx >= 0 && bx < FIELD_WIDTH && by >= 0 && by < FIELD_HEIGHT) {
        cell_char[by][bx] = '*';
        cell_color[by][bx] = COLOR_BOMB;
      }
    }
  }

  // 6. Explosions
  for (int i = 0; i < MAX_EXPLOSIONS; i++) {
    if (game->explosions[i].active) {
      int ex = game->explosions[i].x;
      int ey = game->explosions[i].y;
      if (ey >= 0 && ey < FIELD_HEIGHT) {
        if (ex >= 0 && ex < FIELD_WIDTH) {
          cell_char[ey][ex] = 'X';
          cell_color[ey][ex] = COLOR_EXPLOSION;
        }
      }
    }
  }

  // 7. Player Cannon (Row 22)
  if (game->player.lives > 0) {
    int px = game->player.x;
    if (px - 1 >= 0) {
      cell_char[22][px - 1] = '/';
      cell_color[22][px - 1] = COLOR_PLAYER;
    }
    cell_char[22][px] = '^';
    cell_color[22][px] = "\033[1;38;2;255;255;255m";
    if (px + 1 < FIELD_WIDTH) {
      cell_char[22][px + 1] = '\\';
      cell_color[22][px + 1] = COLOR_PLAYER;
    }
  }

  // Arena Top Border
  offset += snprintf(buffer + offset, sizeof(buffer) - offset, "  %s┌", COLOR_BORDER);
  for (int x = 0; x < FIELD_WIDTH; x++) {
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "─");
  }
  offset += snprintf(buffer + offset, sizeof(buffer) - offset, "┐%s\n", COLOR_RESET);

  // Render Grid lines
  for (int y = 0; y < FIELD_HEIGHT; y++) {
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "  %s│%s", COLOR_BORDER, COLOR_RESET);
    const char *last_col = COLOR_RESET;

    for (int x = 0; x < FIELD_WIDTH; x++) {
      if (cell_color[y][x] != last_col) {
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s", cell_color[y][x]);
        last_col = cell_color[y][x];
      }
      offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%c", cell_char[y][x]);
    }
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s%s│%s\n", COLOR_RESET, COLOR_BORDER, COLOR_RESET);
  }

  // Arena Bottom Border
  offset += snprintf(buffer + offset, sizeof(buffer) - offset, "  %s└", COLOR_BORDER);
  for (int x = 0; x < FIELD_WIDTH; x++) {
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "─");
  }
  offset += snprintf(buffer + offset, sizeof(buffer) - offset, "┘%s\n\n", COLOR_RESET);

  // Status Modals
  if (game->state == STATE_PAUSED) {
    offset += snprintf(
      buffer + offset, sizeof(buffer) - offset,
      "  \033[1;38;2;0;220;255m╔═══════════════════════════════════════════════════╗\033[0m\n"
      "  \033[1;38;2;0;220;255m║                     P A U S E D                   ║\033[0m\n"
      "  \033[1;38;2;0;220;255m║          Press P / ESC to resume defense          ║\033[0m\n"
      "  \033[1;38;2;0;220;255m╚═══════════════════════════════════════════════════╝\033[0m\n\n"
    );
  } else if (game->state == STATE_WAVE_CLEAR) {
    offset += snprintf(
      buffer + offset, sizeof(buffer) - offset,
      "  \033[1;38;2;255;215;0m╔═══════════════════════════════════════════════════╗\033[0m\n"
      "  \033[1;38;2;255;215;0m║        ★ ★ ★   W A V E   C L E A R   ★ ★ ★        ║\033[0m\n"
      "  \033[1;38;2;255;215;0m║             Incoming fleet detected...            ║\033[0m\n"
      "  \033[1;38;2;255;215;0m╚═══════════════════════════════════════════════════╝\033[0m\n\n"
    );
  } else if (game->state == STATE_GAME_OVER) {
    offset += snprintf(
      buffer + offset, sizeof(buffer) - offset,
      "  \033[1;38;2;255;70;70m╔═══════════════════════════════════════════════════╗\033[0m\n"
      "  \033[1;38;2;255;70;70m║                 G A M E   O V E R                 ║\033[0m\n"
      "  \033[1;38;2;255;70;70m║     Score: %-6d   [R] Restart    [Q] Quit        ║\033[0m\n"
      "  \033[1;38;2;255;70;70m╚═══════════════════════════════════════════════════╝\033[0m\n\n",
      game->score
    );
  }

  // Controls Legend
  offset += snprintf(
    buffer + offset, sizeof(buffer) - offset,
    "  %sControls:%s\n"
    "    %s[← / → or A / D]%s Move Cannon    %s[Space / W]%s Fire Laser\n"
    "    %s[P / Esc]%s       Pause           %s[R]%s         Restart    %s[Q]%s Quit\n",
    COLOR_LABEL, COLOR_RESET,
    COLOR_TITLE, COLOR_HINT, COLOR_TITLE, COLOR_HINT,
    COLOR_TITLE, COLOR_HINT, COLOR_TITLE, COLOR_HINT, COLOR_TITLE, COLOR_HINT
  );

  fwrite(buffer, 1, offset, stdout);
  fflush(stdout);
}
