#include "render.h"
#include <stdio.h>
#include <string.h>

#define COLOR_RESET "\033[0m"
#define COLOR_BORDER "\033[38;2;80;115;160m"
#define COLOR_TITLE "\033[1;38;2;255;255;255m"
#define COLOR_HEADER "\033[1;38;2;120;190;255m"
#define COLOR_LABEL "\033[1;38;2;160;180;210m"
#define COLOR_VALUE "\033[1;38;2;255;220;60m"
#define COLOR_GAIN "\033[1;38;2;80;235;120m"
#define COLOR_HINT "\033[38;2;145;155;175m"

void render_init(void) {
  printf("\033[2J\033[H");
  fflush(stdout);
}

void render_cleanup(void) {
  printf(COLOR_RESET);
  fflush(stdout);
}

static void get_tile_colors(int val, const char **bg, const char **fg) {
  switch (val) {
    case 0:
      *bg = "\033[48;2;45;45;58m";
      *fg = "\033[38;2;75;75;95m";
      break;
    case 2:
      *bg = "\033[48;2;238;228;218m";
      *fg = "\033[38;2;119;110;101m";
      break;
    case 4:
      *bg = "\033[48;2;237;224;200m";
      *fg = "\033[38;2;119;110;101m";
      break;
    case 8:
      *bg = "\033[48;2;242;177;121m";
      *fg = "\033[1;38;2;255;255;255m";
      break;
    case 16:
      *bg = "\033[48;2;245;149;99m";
      *fg = "\033[1;38;2;255;255;255m";
      break;
    case 32:
      *bg = "\033[48;2;246;124;95m";
      *fg = "\033[1;38;2;255;255;255m";
      break;
    case 64:
      *bg = "\033[48;2;246;94;59m";
      *fg = "\033[1;38;2;255;255;255m";
      break;
    case 128:
      *bg = "\033[48;2;237;207;114m";
      *fg = "\033[1;38;2;255;255;255m";
      break;
    case 256:
      *bg = "\033[48;2;237;204;97m";
      *fg = "\033[1;38;2;255;255;255m";
      break;
    case 512:
      *bg = "\033[48;2;237;200;80m";
      *fg = "\033[1;38;2;255;255;255m";
      break;
    case 1024:
      *bg = "\033[48;2;0;200;170m";
      *fg = "\033[1;38;2;255;255;255m";
      break;
    case 2048:
      *bg = "\033[48;2;237;194;46m";
      *fg = "\033[1;38;2;255;255;255m";
      break;
    default: // 4096+
      *bg = "\033[48;2;160;60;220m";
      *fg = "\033[1;38;2;255;255;255m";
      break;
  }
}

static void format_tile_middle(int val, char *out, size_t max_len) {
  if (val == 0) {
    snprintf(out, max_len, "   ·    ");
    return;
  }

  char num_str[16];
  snprintf(num_str, sizeof(num_str), "%d", val);
  int len = (int)strlen(num_str);
  if (len > 8) len = 8;
  int left = (8 - len) / 2;
  int right = 8 - len - left;

  snprintf(out, max_len, "%*s%.*s%*s", left, "", len, num_str, right, "");
}

void render_frame(const GameState *game) {
  static char buffer[32768];
  int offset = 0;

  offset += snprintf(buffer + offset, sizeof(buffer) - offset, "\033[H");

  // Title Banner
  offset += snprintf(
    buffer + offset, sizeof(buffer) - offset,
    "  %s╔═════════════════════════════════════════════════════╗%s\n"
    "  %s║%s                  %s2 0 4 8   C L I%s                        %s║%s\n"
    "  %s╚═════════════════════════════════════════════════════╝%s\n\n",
    COLOR_BORDER, COLOR_RESET,
    COLOR_BORDER, COLOR_RESET, COLOR_TITLE, COLOR_RESET, COLOR_BORDER, COLOR_RESET,
    COLOR_BORDER, COLOR_RESET
  );

  // Stats Bar
  char gain_buf[64] = "";
  if (game->last_score_gain > 0) {
    snprintf(gain_buf, sizeof(gain_buf), " %s(+%d)%s", COLOR_GAIN, game->last_score_gain, COLOR_RESET);
  }

  offset += snprintf(
    buffer + offset, sizeof(buffer) - offset,
    "  %sSCORE:%s %s%-6d%s%-10s %sBEST:%s %s%-6d%s  %sMOVES:%s %s%-4d%s  %sUNDO:%s %s%-2d%s\n\n",
    COLOR_LABEL, COLOR_RESET, COLOR_VALUE, game->score, COLOR_RESET, gain_buf,
    COLOR_LABEL, COLOR_RESET, COLOR_VALUE, game->high_score, COLOR_RESET,
    COLOR_LABEL, COLOR_RESET, COLOR_VALUE, game->move_count, COLOR_RESET,
    COLOR_LABEL, COLOR_RESET, COLOR_VALUE, game->history_count, COLOR_RESET
  );

  // Board Top Border
  offset += snprintf(
    buffer + offset, sizeof(buffer) - offset,
    "  %s┌───────────────────────────────────┐%s\n",
    COLOR_BORDER, COLOR_RESET
  );

  // Render 4x4 Grid
  for (int r = 0; r < GRID_SIZE; r++) {
    // 1. Top row of tiles
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "  %s│%s", COLOR_BORDER, COLOR_RESET);
    for (int c = 0; c < GRID_SIZE; c++) {
      const char *bg, *fg;
      get_tile_colors(game->board[r][c], &bg, &fg);
      offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s%s        %s", bg, fg, COLOR_RESET);
      if (c < GRID_SIZE - 1) {
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, " ");
      }
    }
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s│%s\n", COLOR_BORDER, COLOR_RESET);

    // 2. Middle row of tiles (with number)
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "  %s│%s", COLOR_BORDER, COLOR_RESET);
    for (int c = 0; c < GRID_SIZE; c++) {
      const char *bg, *fg;
      get_tile_colors(game->board[r][c], &bg, &fg);
      char mid_text[32];
      format_tile_middle(game->board[r][c], mid_text, sizeof(mid_text));
      offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s%s%s%s", bg, fg, mid_text, COLOR_RESET);
      if (c < GRID_SIZE - 1) {
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, " ");
      }
    }
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s│%s\n", COLOR_BORDER, COLOR_RESET);

    // 3. Bottom row of tiles
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "  %s│%s", COLOR_BORDER, COLOR_RESET);
    for (int c = 0; c < GRID_SIZE; c++) {
      const char *bg, *fg;
      get_tile_colors(game->board[r][c], &bg, &fg);
      offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s%s        %s", bg, fg, COLOR_RESET);
      if (c < GRID_SIZE - 1) {
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, " ");
      }
    }
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s│%s\n", COLOR_BORDER, COLOR_RESET);

    // Separator line between tile rows inside board
    if (r < GRID_SIZE - 1) {
      offset += snprintf(
        buffer + offset, sizeof(buffer) - offset,
        "  %s│                                   │%s\n",
        COLOR_BORDER, COLOR_RESET
      );
    }
  }

  // Board Bottom Border
  offset += snprintf(
    buffer + offset, sizeof(buffer) - offset,
    "  %s└───────────────────────────────────┘%s\n\n",
    COLOR_BORDER, COLOR_RESET
  );

  // Status Modal / Alerts
  if (game->state == STATE_WON) {
    offset += snprintf(
      buffer + offset, sizeof(buffer) - offset,
      "  \033[1;38;2;255;215;0m╔═══════════════════════════════════╗\033[0m\n"
      "  \033[1;38;2;255;215;0m║    ★ ★ ★  YOU REACHED 2048! ★ ★ ★ ║\033[0m\n"
      "  \033[1;38;2;255;215;0m║   [C] Keep Playing   [R] Restart  ║\033[0m\n"
      "  \033[1;38;2;255;215;0m╚═══════════════════════════════════╝\033[0m\n\n"
    );
  } else if (game->state == STATE_GAME_OVER) {
    offset += snprintf(
      buffer + offset, sizeof(buffer) - offset,
      "  \033[1;38;2;255;80;80m╔═══════════════════════════════════╗\033[0m\n"
      "  \033[1;38;2;255;80;80m║          G A M E   O V E R        ║\033[0m\n"
      "  \033[1;38;2;255;80;80m║   [U] Undo Move      [R] Restart  ║\033[0m\n"
      "  \033[1;38;2;255;80;80m╚═══════════════════════════════════╝\033[0m\n\n"
    );
  }

  // Controls Legend
  offset += snprintf(
    buffer + offset, sizeof(buffer) - offset,
    "  %sControls:%s\n"
    "    %s[↑ / W / K]%s Up     %s[↓ / S / J]%s Down\n"
    "    %s[← / A / H]%s Left   %s[→ / D / L]%s Right\n"
    "    %s[U / Z]%s     Undo   %s[R]%s         Restart    %s[Q]%s Quit\n",
    COLOR_LABEL, COLOR_RESET,
    COLOR_TITLE, COLOR_HINT, COLOR_TITLE, COLOR_HINT,
    COLOR_TITLE, COLOR_HINT, COLOR_TITLE, COLOR_HINT,
    COLOR_TITLE, COLOR_HINT, COLOR_TITLE, COLOR_HINT, COLOR_TITLE, COLOR_HINT
  );

  fwrite(buffer, 1, offset, stdout);
  fflush(stdout);
}
