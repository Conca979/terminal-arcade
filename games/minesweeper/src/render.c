#include "render.h"
#include <stdio.h>
#include <string.h>

#define COLOR_RESET "\033[0m"
#define COLOR_BORDER "\033[38;2;75;110;160m"
#define COLOR_TITLE "\033[1;38;2;255;255;255m"
#define COLOR_LABEL "\033[1;38;2;160;180;210m"
#define COLOR_VALUE "\033[1;38;2;255;220;50m"
#define COLOR_HINT "\033[38;2;140;150;170m"
#define COLOR_CURSOR_BG "\033[48;2;60;85;130m"

static const char *NUMBER_COLORS[9] = {
  COLOR_RESET,
  "\033[1;38;2;70;140;255m",  // 1: Blue
  "\033[1;38;2;50;210;50m",   // 2: Green
  "\033[1;38;2;255;70;70m",   // 3: Red
  "\033[1;38;2;60;70;200m",   // 4: Navy
  "\033[1;38;2;180;50;50m",   // 5: Maroon
  "\033[1;38;2;0;210;210m",   // 6: Cyan
  "\033[1;38;2;190;60;220m",  // 7: Purple
  "\033[1;38;2;120;120;130m"  // 8: Gray
};

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
    "  %s╔═══════════════════════════════════════════════════════════╗%s\n"
    "  %s║%s                %sM I N E S W E E P E R   C L I%s                %s║%s\n"
    "  %s╚═══════════════════════════════════════════════════════════╝%s\n\n",
    COLOR_BORDER, COLOR_RESET,
    COLOR_BORDER, COLOR_RESET, COLOR_TITLE, COLOR_RESET, COLOR_BORDER, COLOR_RESET,
    COLOR_BORDER, COLOR_RESET
  );

  // Status Bar: Mines left, Elapsed time, Best time
  int mines_left = game->total_mines - game->flags_placed;
  if (mines_left < 0) mines_left = 0;
  int elapsed = game_get_elapsed_seconds(game, current_time);
  int best = game->best_times[game->difficulty];

  char best_str[32];
  if (best > 0) {
    snprintf(best_str, sizeof(best_str), "%03ds", best);
  } else {
    snprintf(best_str, sizeof(best_str), "--- ");
  }

  const char *diff_names[3] = {"BEGINNER", "INTERMEDIATE", "EXPERT"};
  offset += snprintf(
    buffer + offset, sizeof(buffer) - offset,
    "  %sMINES:%s %s%03d%s   %sTIME:%s %s%03ds%s   %sBEST:%s %s%s%s   %sDIFF:%s \033[1;38;2;0;220;220m[%s]\033[0m\n\n",
    COLOR_LABEL, COLOR_RESET, "\033[1;31m", mines_left, COLOR_RESET,
    COLOR_LABEL, COLOR_RESET, COLOR_VALUE, elapsed, COLOR_RESET,
    COLOR_LABEL, COLOR_RESET, COLOR_VALUE, best_str, COLOR_RESET,
    COLOR_LABEL, COLOR_RESET, diff_names[game->difficulty]
  );

  // Board Top Border
  offset += snprintf(buffer + offset, sizeof(buffer) - offset, "  %s┌", COLOR_BORDER);
  for (int c = 0; c < game->cols; c++) {
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "──");
  }
  offset += snprintf(buffer + offset, sizeof(buffer) - offset, "─┐%s\n", COLOR_RESET);

  // Grid rows
  for (int r = 0; r < game->rows; r++) {
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "  %s│ %s", COLOR_BORDER, COLOR_RESET);

    for (int c = 0; c < game->cols; c++) {
      const Cell *cell = &game->grid[r][c];
      bool is_cursor = (r == game->cursor_r && c == game->cursor_c);

      char cell_str[64];
      if (cell->is_flagged) {
        snprintf(cell_str, sizeof(cell_str), "\033[1;38;2;255;70;70m⚑ %s", COLOR_RESET);
      } else if (!cell->is_revealed) {
        snprintf(cell_str, sizeof(cell_str), "\033[38;2;110;115;130m■ %s", COLOR_RESET);
      } else if (cell->is_mine) {
        if (r == game->hit_mine_r && c == game->hit_mine_c) {
          snprintf(cell_str, sizeof(cell_str), "\033[41;1;97m* %s", COLOR_RESET);
        } else {
          snprintf(cell_str, sizeof(cell_str), "\033[1;31m* %s", COLOR_RESET);
        }
      } else if (cell->neighbor_mines == 0) {
        snprintf(cell_str, sizeof(cell_str), "  ");
      } else {
        snprintf(cell_str, sizeof(cell_str), "%s%d %s", NUMBER_COLORS[cell->neighbor_mines], cell->neighbor_mines, COLOR_RESET);
      }

      if (is_cursor) {
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s%s", COLOR_CURSOR_BG, cell_str);
      } else {
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s", cell_str);
      }
    }

    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s│%s\n", COLOR_BORDER, COLOR_RESET);
  }

  // Board Bottom Border
  offset += snprintf(buffer + offset, sizeof(buffer) - offset, "  %s└", COLOR_BORDER);
  for (int c = 0; c < game->cols; c++) {
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "──");
  }
  offset += snprintf(buffer + offset, sizeof(buffer) - offset, "─┘%s\n\n", COLOR_RESET);

  // Status Modals
  if (game->state == STATE_WON) {
    offset += snprintf(
      buffer + offset, sizeof(buffer) - offset,
      "  \033[1;38;2;255;215;0m╔═══════════════════════════════════╗\033[0m\n"
      "  \033[1;38;2;255;215;0m║    ★ ★ ★  YOU CLEARED IT! ★ ★ ★   ║\033[0m\n"
      "  \033[1;38;2;255;215;0m║   Time: %03ds    [R] Play Again    ║\033[0m\n"
      "  \033[1;38;2;255;215;0m╚═══════════════════════════════════╝\033[0m\n\n",
      elapsed
    );
  } else if (game->state == STATE_GAME_OVER) {
    offset += snprintf(
      buffer + offset, sizeof(buffer) - offset,
      "  \033[1;38;2;255;70;70m╔═══════════════════════════════════╗\033[0m\n"
      "  \033[1;38;2;255;70;70m║       B O O M !  GAME OVER        ║\033[0m\n"
      "  \033[1;38;2;255;70;70m║     [R] Try Again    [Q] Quit     ║\033[0m\n"
      "  \033[1;38;2;255;70;70m╚═══════════════════════════════════╝\033[0m\n\n"
    );
  }

  // Controls Legend
  offset += snprintf(
    buffer + offset, sizeof(buffer) - offset,
    "  %sControls:%s\n"
    "    %s[Arrows / WASD]%s Move Cursor    %s[Space / Enter]%s Reveal Cell\n"
    "    %s[F]%s             Flag Mine      %s[C]%s             Chord Reveal\n"
    "    %s[1 / 2 / 3]%s     Difficulty     %s[R]%s Reset       %s[Q]%s Quit\n",
    COLOR_LABEL, COLOR_RESET,
    COLOR_TITLE, COLOR_HINT, COLOR_TITLE, COLOR_HINT,
    COLOR_TITLE, COLOR_HINT, COLOR_TITLE, COLOR_HINT,
    COLOR_TITLE, COLOR_HINT, COLOR_TITLE, COLOR_HINT, COLOR_TITLE, COLOR_HINT
  );

  fwrite(buffer, 1, offset, stdout);
  fflush(stdout);
}
