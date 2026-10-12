#include "render.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

#define COLOR_RESET "\033[0m"
#define COLOR_COURT_BG "\033[48;2;5;5;10m"
#define COLOR_BORDER "\033[1;38;2;120;140;180m"
#define COLOR_NET "\033[38;2;100;110;130m"
#define COLOR_SCORE "\033[1;38;2;255;255;255m"
#define COLOR_PLAYER "\033[1;38;2;0;230;255m"
#define COLOR_PLAYER_FLASH "\033[1;38;2;255;255;200m"
#define COLOR_CPU "\033[1;38;2;255;90;120m"
#define COLOR_CPU_FLASH "\033[1;38;2;255;255;200m"
#define COLOR_BALL "\033[1;38;2;255;255;255m"
#define COLOR_TRAIL_0 "\033[38;2;200;210;230m"
#define COLOR_TRAIL_1 "\033[38;2;140;150;180m"
#define COLOR_TRAIL_2 "\033[38;2;90;100;130m"
#define COLOR_TRAIL_3 "\033[38;2;50;60;80m"
#define COLOR_TITLE "\033[1;38;2;255;255;255m"
#define COLOR_LABEL "\033[38;2;140;160;190m"
#define COLOR_VALUE "\033[1;38;2;255;215;60m"
#define COLOR_ACCENT "\033[1;38;2;0;220;210m"
#define COLOR_BANNER "\033[1;38;2;255;230;80;48;2;20;30;50m"

static const char *DIGITS_5x4[10][5] = {
  {"████", "█  █", "█  █", "█  █", "████"}, // 0
  {"  ██", "   █", "   █", "   █", "  ██"}, // 1
  {"████", "   █", "████", "█   ", "████"}, // 2
  {"████", "   █", "████", "   █", "████"}, // 3
  {"█  █", "█  █", "████", "   █", "   █"}, // 4
  {"████", "█   ", "████", "   █", "████"}, // 5
  {"████", "█   ", "████", "█  █", "████"}, // 6
  {"████", "   █", "   █", "   █", "   █"}, // 7
  {"████", "█  █", "████", "█  █", "████"}, // 8
  {"████", "█  █", "████", "   █", "████"}  // 9
};

void render_init(void) {
  printf("\033[2J\033[H");
  fflush(stdout);
}

void render_cleanup(void) {
  printf(COLOR_RESET);
  fflush(stdout);
}

static void draw_digit_to_grid(int grid[COURT_HEIGHT][COURT_WIDTH], int digit, int start_r, int start_c, int type) {
  if (digit < 0 || digit > 9) return;
  for (int r = 0; r < 5; r++) {
    for (int c = 0; c < 4; c++) {
      int gr = start_r + r;
      int gc = start_c + c;
      if (gr >= 0 && gr < COURT_HEIGHT && gc >= 0 && gc < COURT_WIDTH) {
        if (DIGITS_5x4[digit][r][c] != ' ') {
          grid[gr][gc] = type;
        }
      }
    }
  }
}

void render_frame(const GameState *game) {
  static char buffer[65536];
  int offset = 0;

  offset += snprintf(buffer + offset, sizeof(buffer) - offset, "\033[H");

  // Header Banner
  offset += snprintf(
    buffer + offset, sizeof(buffer) - offset,
    "  %s╔════════════════════════════════════════════════════════════════════════════╗%s\n"
    "  %s║%s                      %sC L A S S I C   P O N G   ( C )%s                     %s║%s\n"
    "  %s╚════════════════════════════════════════════════════════════════════════════╝%s\n",
    COLOR_BORDER, COLOR_RESET,
    COLOR_BORDER, COLOR_RESET, COLOR_TITLE, COLOR_RESET, COLOR_BORDER, COLOR_RESET,
    COLOR_BORDER, COLOR_RESET
  );

  // Cell types:
  // 0 = empty
  // 1 = top/bottom border
  // 2 = net dash
  // 3 = score digit
  // 4 = player paddle
  // 5 = cpu paddle
  // 6 = ball
  // 7 = trail 0 (newest)
  // 8 = trail 1
  // 9 = trail 2
  // 10 = trail 3 (oldest)
  static int grid[COURT_HEIGHT][COURT_WIDTH];
  memset(grid, 0, sizeof(grid));

  // 1. Borders
  for (int c = 0; c < COURT_WIDTH; c++) {
    grid[0][c] = 1;
    grid[COURT_HEIGHT - 1][c] = 1;
  }

  // 2. Net
  int net_x = COURT_WIDTH / 2;
  for (int r = 1; r < COURT_HEIGHT - 1; r++) {
    if (r % 2 == 1) {
      grid[r][net_x] = 2;
    }
  }

  // 3. Scoreboard digits
  // Player Score (left)
  if (game->player_score < 10) {
    draw_digit_to_grid(grid, game->player_score, 2, 28, 3);
  } else {
    draw_digit_to_grid(grid, game->player_score / 10, 2, 25, 3);
    draw_digit_to_grid(grid, game->player_score % 10, 2, 30, 3);
  }

  // CPU Score (right)
  if (game->cpu_score < 10) {
    draw_digit_to_grid(grid, game->cpu_score, 2, 44, 3);
  } else {
    draw_digit_to_grid(grid, game->cpu_score / 10, 2, 42, 3);
    draw_digit_to_grid(grid, game->cpu_score % 10, 2, 47, 3);
  }

  // 4. Ball Trail
  if (game->state == STATE_PLAYING) {
    for (int i = game->ball.trail_count - 1; i >= 0; i--) {
      int tr = (int)roundf(game->ball.trail[i].y);
      int tc = (int)roundf(game->ball.trail[i].x);
      if (tr >= 1 && tr < COURT_HEIGHT - 1 && tc >= 1 && tc < COURT_WIDTH - 1) {
        int trail_type = 7 + i;
        if (trail_type > 10) trail_type = 10;
        grid[tr][tc] = trail_type;
      }
    }
  }

  // 5. Ball
  if (game->state == STATE_PLAYING || game->state == STATE_PAUSED || game->state == STATE_POINT_SCORED) {
    int br = (int)roundf(game->ball.y);
    int bc = (int)roundf(game->ball.x);
    if (br >= 1 && br < COURT_HEIGHT - 1 && bc >= 0 && bc < COURT_WIDTH) {
      grid[br][bc] = 6;
    }
  }

  // 6. Player Paddle (col = 2)
  int pr = (int)roundf(game->player.y);
  for (int dy = -2; dy <= 2; dy++) {
    int r = pr + dy;
    if (r >= 1 && r < COURT_HEIGHT - 1) {
      grid[r][2] = 4;
    }
  }

  // 7. CPU Paddle (col = COURT_WIDTH - 3)
  int cr = (int)roundf(game->cpu.y);
  for (int dy = -2; dy <= 2; dy++) {
    int r = cr + dy;
    if (r >= 1 && r < COURT_HEIGHT - 1) {
      grid[r][COURT_WIDTH - 3] = 5;
    }
  }

  // Render Court
  for (int r = 0; r < COURT_HEIGHT; r++) {
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "  ");

    for (int c = 0; c < COURT_WIDTH; c++) {
      int cell = grid[r][c];
      switch (cell) {
        case 1: // Top / Bottom border
          offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s═%s", COLOR_BORDER, COLOR_RESET);
          break;

        case 2: // Net dash
          offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s│%s", COLOR_NET, COLOR_RESET);
          break;

        case 3: // Score digit
          offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s█%s", COLOR_SCORE, COLOR_RESET);
          break;

        case 4: // Player paddle
          if (game->player.hit_flash > 0) {
            offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s█%s", COLOR_PLAYER_FLASH, COLOR_RESET);
          } else {
            offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s█%s", COLOR_PLAYER, COLOR_RESET);
          }
          break;

        case 5: // CPU paddle
          if (game->cpu.hit_flash > 0) {
            offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s█%s", COLOR_CPU_FLASH, COLOR_RESET);
          } else {
            offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s█%s", COLOR_CPU, COLOR_RESET);
          }
          break;

        case 6: // Ball
          offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s█%s", COLOR_BALL, COLOR_RESET);
          break;

        case 7: // Trail 0 (brightest)
          offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s█%s", COLOR_TRAIL_0, COLOR_RESET);
          break;

        case 8: // Trail 1
          offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s▒%s", COLOR_TRAIL_1, COLOR_RESET);
          break;

        case 9: // Trail 2
          offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s░%s", COLOR_TRAIL_2, COLOR_RESET);
          break;

        case 10: // Trail 3 (oldest)
          offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s·%s", COLOR_TRAIL_3, COLOR_RESET);
          break;

        default:
          offset += snprintf(buffer + offset, sizeof(buffer) - offset, " ");
          break;
      }
    }
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "\n");
  }

  // Overlay Modals / Banners below court
  const char *diff_str = "MEDIUM";
  if (game->difficulty == DIFF_EASY) diff_str = "EASY";
  else if (game->difficulty == DIFF_HARD) diff_str = "HARD";

  if (game->state == STATE_SERVE) {
    offset += snprintf(
      buffer + offset, sizeof(buffer) - offset,
      "  %s               >>> PRESS SPACE OR ENTER TO SERVE BALL <<<               %s\n",
      COLOR_BANNER, COLOR_RESET
    );
  } else if (game->state == STATE_PAUSED) {
    offset += snprintf(
      buffer + offset, sizeof(buffer) - offset,
      "  %s                    >>> GAME PAUSED - PRESS P / SPACE <<<               %s\n",
      COLOR_BANNER, COLOR_RESET
    );
  } else if (game->state == STATE_POINT_SCORED) {
    if (game->scorer == 1) {
      offset += snprintf(
        buffer + offset, sizeof(buffer) - offset,
        "  %s                  ★ POINT SCORED BY PLAYER! READY TO SERVE ★             %s\n",
        COLOR_BANNER, COLOR_RESET
      );
    } else {
      offset += snprintf(
        buffer + offset, sizeof(buffer) - offset,
        "  %s                    ● POINT SCORED BY CPU! READY TO SERVE ●              %s\n",
        COLOR_BANNER, COLOR_RESET
      );
    }
  } else if (game->state == STATE_GAME_OVER) {
    if (game->player_score >= WIN_SCORE) {
      offset += snprintf(
        buffer + offset, sizeof(buffer) - offset,
        "  %s        ★ VICTORY! YOU DEFEATED THE CPU! PRESS [R] OR [SPACE] TO REPLAY ★%s\n",
        COLOR_BANNER, COLOR_RESET
      );
    } else {
      offset += snprintf(
        buffer + offset, sizeof(buffer) - offset,
        "  %s          CPU WINS MATCH! BETTER LUCK NEXT TIME! PRESS [R] TO RETRY      %s\n",
        COLOR_BANNER, COLOR_RESET
      );
    }
  } else {
    offset += snprintf(
      buffer + offset, sizeof(buffer) - offset,
      "  %sRALLY STREAK:%s %s%02d%s   %sBEST RALLY:%s %s%02d%s   %sAI DIFFICULTY:%s %s%-6s%s   %sTARGET:%s %s%d PTS%s\n",
      COLOR_LABEL, COLOR_RESET, COLOR_VALUE, game->rally_count, COLOR_RESET,
      COLOR_LABEL, COLOR_RESET, COLOR_VALUE, game->max_rally, COLOR_RESET,
      COLOR_LABEL, COLOR_RESET, COLOR_ACCENT, diff_str, COLOR_RESET,
      COLOR_LABEL, COLOR_RESET, COLOR_VALUE, WIN_SCORE, COLOR_RESET
    );
  }

  // Footer Controls Reference
  offset += snprintf(
    buffer + offset, sizeof(buffer) - offset,
    "\n  %sControls:%s %s[W / S]%s or %s[↑ / ↓]%s Move Paddle  │  %s[Space]%s Serve / Pause  │  %s[Tab]%s AI Diff  │  %s[R]%s Reset  │  %s[Q]%s Quit\n",
    COLOR_LABEL, COLOR_RESET,
    COLOR_TITLE, COLOR_RESET, COLOR_TITLE, COLOR_RESET,
    COLOR_TITLE, COLOR_RESET,
    COLOR_TITLE, COLOR_RESET,
    COLOR_TITLE, COLOR_RESET,
    COLOR_TITLE, COLOR_RESET
  );

  fwrite(buffer, 1, offset, stdout);
  fflush(stdout);
}
