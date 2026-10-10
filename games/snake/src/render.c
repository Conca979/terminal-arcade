#include "render.h"
#include <stdio.h>
#include <string.h>

#define COLOR_RESET "\033[0m"
#define COLOR_TITLE "\033[1;38;2;255;255;255m"
#define COLOR_LABEL "\033[1;38;2;160;180;210m"
#define COLOR_VALUE "\033[1;38;2;255;220;50m"
#define COLOR_HINT "\033[38;2;140;150;170m"
#define COLOR_GRID "\033[38;2;45;35;55m"

#define COLOR_BORDER_NORMAL "\033[38;2;70;120;190m"
#define COLOR_BORDER_WRAP "\033[38;2;0;220;200m"
#define COLOR_BORDER_PHASE "\033[1;38;2;255;60;220m"

void render_init(void) {
  printf("\033[2J\033[H");
  fflush(stdout);
}

void render_cleanup(void) {
  printf(COLOR_RESET);
  fflush(stdout);
}

void render_frame(const GameState *game, unsigned long long current_time) {
  static char buffer[32768];
  int offset = 0;

  offset += snprintf(buffer + offset, sizeof(buffer) - offset, "\033[H");

  // Title Banner
  offset += snprintf(
    buffer + offset, sizeof(buffer) - offset,
    "  \033[38;2;0;220;240m╔════════════════════════════════════════════════════════════════╗%s\n"
    "  \033[38;2;0;220;240m║%s           \033[1;38;2;255;255;255mC Y B E R P U N K   S N A K E   ( C )\033[0m              \033[38;2;0;220;240m║%s\n"
    "  \033[38;2;0;220;240m╚════════════════════════════════════════════════════════════════╝%s\n\n",
    COLOR_RESET, COLOR_RESET, COLOR_RESET, COLOR_RESET
  );

  // Active Buffs Bar
  char buff_str[128] = "";
  if (game->overclock_active) {
    int rem_ms = (int)(game->overclock_expire - current_time);
    if (rem_ms < 0) rem_ms = 0;
    snprintf(buff_str + strlen(buff_str), sizeof(buff_str) - strlen(buff_str),
             " \033[1;38;2;0;255;255m[⚡ OVERCLOCK 2x: %.1fs]\033[0m", rem_ms / 1000.0);
  }
  if (game->phase_active) {
    int rem_ms = (int)(game->phase_expire - current_time);
    if (rem_ms < 0) rem_ms = 0;
    snprintf(buff_str + strlen(buff_str), sizeof(buff_str) - strlen(buff_str),
             " \033[1;38;2;255;60;220m[◎ PHASE SHIFT: %.1fs]\033[0m", rem_ms / 1000.0);
  }

  const char *mode_str = game->wrap_mode ? "\033[1;38;2;0;220;200m[WRAP-AROUND]\033[0m" : "\033[1;38;2;255;100;100m[SOLID WALLS]\033[0m";

  offset += snprintf(
    buffer + offset, sizeof(buffer) - offset,
    "  %sSCORE:%s %s%-6d%s  %sBEST:%s %s%-6d%s  %sLEN:%s %s%-3d%s  %sMODE:%s %s%s\n\n",
    COLOR_LABEL, COLOR_RESET, COLOR_VALUE, game->score, COLOR_RESET,
    COLOR_LABEL, COLOR_RESET, COLOR_VALUE, game->high_score, COLOR_RESET,
    COLOR_LABEL, COLOR_RESET, COLOR_VALUE, game->length, COLOR_RESET,
    COLOR_LABEL, COLOR_RESET, mode_str, buff_str
  );

  // Border color based on phase / wrap mode
  const char *border_color = COLOR_BORDER_NORMAL;
  if (game->phase_active) {
    border_color = COLOR_BORDER_PHASE;
  } else if (game->wrap_mode) {
    border_color = COLOR_BORDER_WRAP;
  }

  // Arena Top Border
  offset += snprintf(buffer + offset, sizeof(buffer) - offset, "  %s╔", border_color);
  for (int c = 0; c < ARENA_WIDTH; c++) {
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "══");
  }
  offset += snprintf(buffer + offset, sizeof(buffer) - offset, "╗%s\n", COLOR_RESET);

  // Render Grid rows
  for (int y = 0; y < ARENA_HEIGHT; y++) {
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "  %s║%s", border_color, COLOR_RESET);

    for (int x = 0; x < ARENA_WIDTH; x++) {
      // 1. Is snake head?
      if (game->body[0].x == x && game->body[0].y == y) {
        const char *head_sym = "██";
        if (game->dir == DIR_UP) head_sym = "▲ ";
        else if (game->dir == DIR_DOWN) head_sym = "▼ ";
        else if (game->dir == DIR_LEFT) head_sym = "◄ ";
        else if (game->dir == DIR_RIGHT) head_sym = "► ";

        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "\033[1;38;2;255;255;255;48;2;0;180;220m%s%s", head_sym, COLOR_RESET);
        continue;
      }

      // 2. Is snake body segment?
      int body_idx = -1;
      for (int i = 1; i < game->length; i++) {
        if (game->body[i].x == x && game->body[i].y == y) {
          body_idx = i;
          break;
        }
      }

      if (body_idx > 0) {
        // Gradient from Cyan (0, 240, 255) -> Violet (140, 60, 240) -> Hot Pink (255, 50, 190)
        float t = (float)body_idx / (float)(game->length > 1 ? game->length - 1 : 1);
        int r = (int)(t * 255.0f);
        int g = (int)(240.0f - t * 190.0f);
        int b = (int)(255.0f - t * 65.0f);
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "\033[38;2;%d;%d;%dm██%s", r, g, b, COLOR_RESET);
        continue;
      }

      // 3. Regular food
      if (game->regular_food.type == FOOD_DATA &&
          game->regular_food.pos.x == x && game->regular_food.pos.y == y) {
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "\033[1;38;2;50;255;120m◆ %s", COLOR_RESET);
        continue;
      }

      // 4. Bonus food
      if (game->bonus_food.type != FOOD_NONE &&
          game->bonus_food.pos.x == x && game->bonus_food.pos.y == y) {
        switch (game->bonus_food.type) {
          case FOOD_CORE:
            offset += snprintf(buffer + offset, sizeof(buffer) - offset, "\033[1;38;2;255;215;0m★ %s", COLOR_RESET);
            break;
          case FOOD_OVERCLOCK:
            offset += snprintf(buffer + offset, sizeof(buffer) - offset, "\033[1;38;2;0;255;255m⚡%s ", COLOR_RESET);
            break;
          case FOOD_PHASE:
            offset += snprintf(buffer + offset, sizeof(buffer) - offset, "\033[1;38;2;255;60;220m◎ %s", COLOR_RESET);
            break;
          default:
            offset += snprintf(buffer + offset, sizeof(buffer) - offset, "  ");
            break;
        }
        continue;
      }

      // 5. Empty cell
      offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s ·%s", COLOR_GRID, COLOR_RESET);
    }

    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s║%s\n", border_color, COLOR_RESET);
  }

  // Arena Bottom Border
  offset += snprintf(buffer + offset, sizeof(buffer) - offset, "  %s╚", border_color);
  for (int c = 0; c < ARENA_WIDTH; c++) {
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "══");
  }
  offset += snprintf(buffer + offset, sizeof(buffer) - offset, "╝%s\n\n", COLOR_RESET);

  // Status Overlays
  if (game->state == STATE_PAUSED) {
    offset += snprintf(
      buffer + offset, sizeof(buffer) - offset,
      "  \033[1;38;2;0;220;255m╔═══════════════════════════════════════════════════╗\033[0m\n"
      "  \033[1;38;2;0;220;255m║                     P A U S E D                   ║\033[0m\n"
      "  \033[1;38;2;0;220;255m║          Press P / ESC to resume light-cycle      ║\033[0m\n"
      "  \033[1;38;2;0;220;255m╚═══════════════════════════════════════════════════╝\033[0m\n\n"
    );
  } else if (game->state == STATE_GAME_OVER) {
    offset += snprintf(
      buffer + offset, sizeof(buffer) - offset,
      "  \033[1;38;2;255;60;80m╔═══════════════════════════════════════════════════╗\033[0m\n"
      "  \033[1;38;2;255;60;80m║            S Y S T E M   C R A S H                ║\033[0m\n"
      "  \033[1;38;2;255;60;80m║    Score: %-6d  Length: %-3d    [R] Restart       ║\033[0m\n"
      "  \033[1;38;2;255;60;80m╚═══════════════════════════════════════════════════╝\033[0m\n\n",
      game->score, game->length
    );
  }

  // Controls Legend
  offset += snprintf(
    buffer + offset, sizeof(buffer) - offset,
    "  %sControls:%s\n"
    "    %s[Arrows / WASD / HJKL]%s Steer    %s[M]%s Toggle Wrap Mode\n"
    "    %s[P / Esc]%s              Pause    %s[R]%s Restart    %s[Q]%s Quit\n",
    COLOR_LABEL, COLOR_RESET,
    COLOR_TITLE, COLOR_HINT, COLOR_TITLE, COLOR_HINT,
    COLOR_TITLE, COLOR_HINT, COLOR_TITLE, COLOR_HINT, COLOR_TITLE, COLOR_HINT
  );

  fwrite(buffer, 1, offset, stdout);
  fflush(stdout);
}
