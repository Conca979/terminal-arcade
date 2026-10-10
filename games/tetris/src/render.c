#include "render.h"
#include <stdio.h>
#include <string.h>

#define COLOR_RESET "\033[0m"
#define COLOR_BORDER "\033[38;2;75;110;160m"
#define COLOR_HEADER "\033[1;38;2;100;180;255m"
#define COLOR_TITLE "\033[1;38;2;255;255;255m"
#define COLOR_LABEL "\033[1;38;2;160;180;210m"
#define COLOR_VALUE "\033[1;38;2;255;220;50m"
#define COLOR_HINT "\033[38;2;140;150;170m"
#define COLOR_GHOST "\033[38;2;90;90;105m"
#define COLOR_GRID "\033[38;2;50;50;65m"
#define COLOR_FLASH "\033[1;97;107m"
#define COLOR_MODAL_BG "\033[1;38;2;255;255;255;48;2;30;30;45m"
#define COLOR_MODAL_TEXT "\033[1;38;2;255;230;80m"

static const char *PIECE_COLORS[TET_COUNT] = {
  COLOR_RESET,
  "\033[38;2;0;235;235m",   // I - Cyan
  "\033[38;2;70;140;255m",  // J - Blue
  "\033[38;2;255;155;0m",   // L - Orange
  "\033[38;2;255;225;0m",   // O - Yellow
  "\033[38;2;50;215;50m",   // S - Green
  "\033[38;2;195;65;255m",  // T - Purple
  "\033[38;2;255;65;65m"    // Z - Red
};

void render_init(void) {
  // Clear screen initially
  printf("\033[2J\033[H");
  fflush(stdout);
}

void render_cleanup(void) {
  printf(COLOR_RESET);
  fflush(stdout);
}

static void get_mini_row(TetrominoType type, int row, char *out, size_t max_len) {
  const char *color = PIECE_COLORS[type];
  switch (type) {
    case TET_I:
      if (row == 0) {
        snprintf(out, max_len, "%s████████%s", color, COLOR_RESET);
      } else {
        snprintf(out, max_len, "        ");
      }
      break;

    case TET_J:
      if (row == 0) {
        snprintf(out, max_len, "  %s██%s      ", color, COLOR_RESET);
      } else {
        snprintf(out, max_len, "  %s██████%s  ", color, COLOR_RESET);
      }
      break;

    case TET_L:
      if (row == 0) {
        snprintf(out, max_len, "      %s██%s  ", color, COLOR_RESET);
      } else {
        snprintf(out, max_len, "  %s██████%s  ", color, COLOR_RESET);
      }
      break;

    case TET_O:
      if (row == 0) {
        snprintf(out, max_len, "  %s████%s    ", color, COLOR_RESET);
      } else {
        snprintf(out, max_len, "  %s████%s    ", color, COLOR_RESET);
      }
      break;

    case TET_S:
      if (row == 0) {
        snprintf(out, max_len, "    %s████%s  ", color, COLOR_RESET);
      } else {
        snprintf(out, max_len, "  %s████%s    ", color, COLOR_RESET);
      }
      break;

    case TET_T:
      if (row == 0) {
        snprintf(out, max_len, "    %s██%s    ", color, COLOR_RESET);
      } else {
        snprintf(out, max_len, "  %s██████%s  ", color, COLOR_RESET);
      }
      break;

    case TET_Z:
      if (row == 0) {
        snprintf(out, max_len, "  %s████%s    ", color, COLOR_RESET);
      } else {
        snprintf(out, max_len, "    %s████%s  ", color, COLOR_RESET);
      }
      break;

    default:
      snprintf(out, max_len, "        ");
      break;
  }
}

void render_frame(const GameState *game) {
  static char buffer[32768];
  int offset = 0;

  // Move cursor to top-left
  offset += snprintf(buffer + offset, sizeof(buffer) - offset, "\033[H");

  // Title Banner
  offset += snprintf(
    buffer + offset, sizeof(buffer) - offset,
    "  %s╔═══════════════════════════════════════════════════════════════╗%s\n"
    "  %s║%s                  %sT E T R I S   C L I%s                          %s║%s\n"
    "  %s╚═══════════════════════════════════════════════════════════════╝%s\n\n",
    COLOR_BORDER, COLOR_RESET,
    COLOR_BORDER, COLOR_RESET, COLOR_TITLE, COLOR_RESET, COLOR_BORDER, COLOR_RESET,
    COLOR_BORDER, COLOR_RESET
  );

  // Active and Ghost blocks
  Point current_blocks[4];
  game_get_piece_blocks(game->current.type, game->current.rotation, current_blocks);
  int ghost_y = game_get_ghost_y(game);

  // Mini preview buffers
  char hold_r0[64], hold_r1[64];
  get_mini_row(game->hold_piece, 0, hold_r0, sizeof(hold_r0));
  get_mini_row(game->hold_piece, 1, hold_r1, sizeof(hold_r1));

  char next0_r0[64], next0_r1[64];
  char next1_r0[64], next1_r1[64];
  char next2_r0[64], next2_r1[64];
  get_mini_row(game->next_queue[0], 0, next0_r0, sizeof(next0_r0));
  get_mini_row(game->next_queue[0], 1, next0_r1, sizeof(next0_r1));
  get_mini_row(game->next_queue[1], 0, next1_r0, sizeof(next1_r0));
  get_mini_row(game->next_queue[1], 1, next1_r1, sizeof(next1_r1));
  get_mini_row(game->next_queue[2], 0, next2_r0, sizeof(next2_r0));
  get_mini_row(game->next_queue[2], 1, next2_r1, sizeof(next2_r1));

  // Loop through 22 vertical lines of the main game display
  for (int line = 0; line < 22; line++) {
    // 1. LEFT PANEL (Width ~14)
    char left_part[256];
    switch (line) {
      case 0:
        snprintf(left_part, sizeof(left_part), "  %s┌── HOLD ──┐%s", COLOR_BORDER, COLOR_RESET);
        break;
      case 1:
        snprintf(left_part, sizeof(left_part), "  %s│%s          %s│%s", COLOR_BORDER, COLOR_RESET, COLOR_BORDER, COLOR_RESET);
        break;
      case 2:
        snprintf(left_part, sizeof(left_part), "  %s│%s %s %s│%s", COLOR_BORDER, COLOR_RESET, hold_r0, COLOR_BORDER, COLOR_RESET);
        break;
      case 3:
        snprintf(left_part, sizeof(left_part), "  %s│%s %s %s│%s", COLOR_BORDER, COLOR_RESET, hold_r1, COLOR_BORDER, COLOR_RESET);
        break;
      case 4:
        snprintf(left_part, sizeof(left_part), "  %s│%s          %s│%s", COLOR_BORDER, COLOR_RESET, COLOR_BORDER, COLOR_RESET);
        break;
      case 5:
        snprintf(left_part, sizeof(left_part), "  %s└──────────┘%s", COLOR_BORDER, COLOR_RESET);
        break;
      case 6:
        snprintf(left_part, sizeof(left_part), "              ");
        break;
      case 7:
        snprintf(left_part, sizeof(left_part), "  %s┌── STATS ─┐%s", COLOR_BORDER, COLOR_RESET);
        break;
      case 8:
        snprintf(left_part, sizeof(left_part), "  %s│%s %sSCORE%s    %s│%s", COLOR_BORDER, COLOR_RESET, COLOR_LABEL, COLOR_RESET, COLOR_BORDER, COLOR_RESET);
        break;
      case 9:
        snprintf(left_part, sizeof(left_part), "  %s│%s  %s%06d%s  %s│%s", COLOR_BORDER, COLOR_RESET, COLOR_VALUE, game->score, COLOR_RESET, COLOR_BORDER, COLOR_RESET);
        break;
      case 10:
        snprintf(left_part, sizeof(left_part), "  %s│%s          %s│%s", COLOR_BORDER, COLOR_RESET, COLOR_BORDER, COLOR_RESET);
        break;
      case 11:
        snprintf(left_part, sizeof(left_part), "  %s│%s %sHIGH%s     %s│%s", COLOR_BORDER, COLOR_RESET, COLOR_LABEL, COLOR_RESET, COLOR_BORDER, COLOR_RESET);
        break;
      case 12:
        snprintf(left_part, sizeof(left_part), "  %s│%s  %s%06d%s  %s│%s", COLOR_BORDER, COLOR_RESET, COLOR_VALUE, game->high_score, COLOR_RESET, COLOR_BORDER, COLOR_RESET);
        break;
      case 13:
        snprintf(left_part, sizeof(left_part), "  %s│%s          %s│%s", COLOR_BORDER, COLOR_RESET, COLOR_BORDER, COLOR_RESET);
        break;
      case 14:
        snprintf(left_part, sizeof(left_part), "  %s│%s %sLEVEL%s    %s│%s", COLOR_BORDER, COLOR_RESET, COLOR_LABEL, COLOR_RESET, COLOR_BORDER, COLOR_RESET);
        break;
      case 15:
        snprintf(left_part, sizeof(left_part), "  %s│%s    %s%-4d%s  %s│%s", COLOR_BORDER, COLOR_RESET, COLOR_VALUE, game->level, COLOR_RESET, COLOR_BORDER, COLOR_RESET);
        break;
      case 16:
        snprintf(left_part, sizeof(left_part), "  %s│%s          %s│%s", COLOR_BORDER, COLOR_RESET, COLOR_BORDER, COLOR_RESET);
        break;
      case 17:
        snprintf(left_part, sizeof(left_part), "  %s│%s %sLINES%s    %s│%s", COLOR_BORDER, COLOR_RESET, COLOR_LABEL, COLOR_RESET, COLOR_BORDER, COLOR_RESET);
        break;
      case 18:
        snprintf(left_part, sizeof(left_part), "  %s│%s    %s%-4d%s  %s│%s", COLOR_BORDER, COLOR_RESET, COLOR_VALUE, game->lines_cleared, COLOR_RESET, COLOR_BORDER, COLOR_RESET);
        break;
      case 19:
        snprintf(left_part, sizeof(left_part), "  %s└──────────┘%s", COLOR_BORDER, COLOR_RESET);
        break;
      default:
        snprintf(left_part, sizeof(left_part), "              ");
        break;
    }

    // 2. MIDDLE PLAYFIELD (Width 22)
    char playfield_part[1024];
    if (line == 0) {
      snprintf(playfield_part, sizeof(playfield_part), "    %s┌────────────────────┐%s", COLOR_BORDER, COLOR_RESET);
    } else if (line == 21) {
      snprintf(playfield_part, sizeof(playfield_part), "    %s└────────────────────┘%s", COLOR_BORDER, COLOR_RESET);
    } else {
      int visible_y = line - 1; // 0 to 19
      int board_y = visible_y + BUFFER_HEIGHT; // 2 to 21

      // Check if paused or game over overlay applies to this row
      if (game->state == STATE_PAUSED && visible_y >= 8 && visible_y <= 12) {
        if (visible_y == 8) {
          snprintf(playfield_part, sizeof(playfield_part), "    %s│%s    %s┌────────────┐%s    %s│%s", COLOR_BORDER, COLOR_RESET, COLOR_HEADER, COLOR_RESET, COLOR_BORDER, COLOR_RESET);
        } else if (visible_y == 9) {
          snprintf(playfield_part, sizeof(playfield_part), "    %s│%s    %s│%s   PAUSED   %s│%s    %s│%s", COLOR_BORDER, COLOR_RESET, COLOR_HEADER, COLOR_MODAL_TEXT, COLOR_HEADER, COLOR_RESET, COLOR_BORDER, COLOR_RESET);
        } else if (visible_y == 10) {
          snprintf(playfield_part, sizeof(playfield_part), "    %s│%s    %s│%s Press P/ESC %s│%s    %s│%s", COLOR_BORDER, COLOR_RESET, COLOR_HEADER, COLOR_TITLE, COLOR_HEADER, COLOR_RESET, COLOR_BORDER, COLOR_RESET);
        } else if (visible_y == 11) {
          snprintf(playfield_part, sizeof(playfield_part), "    %s│%s    %s│%s to resume  %s│%s    %s│%s", COLOR_BORDER, COLOR_RESET, COLOR_HEADER, COLOR_HINT, COLOR_HEADER, COLOR_RESET, COLOR_BORDER, COLOR_RESET);
        } else {
          snprintf(playfield_part, sizeof(playfield_part), "    %s│%s    %s└────────────┘%s    %s│%s", COLOR_BORDER, COLOR_RESET, COLOR_HEADER, COLOR_RESET, COLOR_BORDER, COLOR_RESET);
        }
      } else if (game->state == STATE_GAME_OVER && visible_y >= 8 && visible_y <= 12) {
        if (visible_y == 8) {
          snprintf(playfield_part, sizeof(playfield_part), "    %s│%s    %s┌────────────┐%s    %s│%s", COLOR_BORDER, COLOR_RESET, "\033[1;31m", COLOR_RESET, COLOR_BORDER, COLOR_RESET);
        } else if (visible_y == 9) {
          snprintf(playfield_part, sizeof(playfield_part), "    %s│%s    %s│%s  GAME OVER %s│%s    %s│%s", COLOR_BORDER, COLOR_RESET, "\033[1;31m", "\033[1;31m", "\033[1;31m", COLOR_RESET, COLOR_BORDER, COLOR_RESET);
        } else if (visible_y == 10) {
          snprintf(playfield_part, sizeof(playfield_part), "    %s│%s    %s│%s R: Play New%s│%s    %s│%s", COLOR_BORDER, COLOR_RESET, "\033[1;31m", COLOR_TITLE, "\033[1;31m", COLOR_RESET, COLOR_BORDER, COLOR_RESET);
        } else if (visible_y == 11) {
          snprintf(playfield_part, sizeof(playfield_part), "    %s│%s    %s│%s Q: Quit Game%s│%s   %s│%s", COLOR_BORDER, COLOR_RESET, "\033[1;31m", COLOR_HINT, "\033[1;31m", COLOR_RESET, COLOR_BORDER, COLOR_RESET);
        } else {
          snprintf(playfield_part, sizeof(playfield_part), "    %s│%s    %s└────────────┘%s    %s│%s", COLOR_BORDER, COLOR_RESET, "\033[1;31m", COLOR_RESET, COLOR_BORDER, COLOR_RESET);
        }
      } else {
        // Normal row rendering
        int row_offset = 0;
        row_offset += snprintf(playfield_part + row_offset, sizeof(playfield_part) - row_offset, "    %s│%s", COLOR_BORDER, COLOR_RESET);

        // Check if row is currently flashing in line clear
        bool is_clearing = false;
        if (game->state == STATE_LINE_CLEAR) {
          for (int c = 0; c < game->clearing_row_count; c++) {
            if (game->clearing_rows[c] == board_y) {
              is_clearing = true;
              break;
            }
          }
        }

        if (is_clearing) {
          row_offset += snprintf(playfield_part + row_offset, sizeof(playfield_part) - row_offset, "%s████████████████████%s", COLOR_FLASH, COLOR_RESET);
        } else {
          for (int x = 0; x < BOARD_WIDTH; x++) {
            // 1. Is active piece block?
            bool is_active = false;
            if (game->state == STATE_PLAYING || game->state == STATE_PAUSED) {
              for (int b = 0; b < 4; b++) {
                if (game->current.x + current_blocks[b].x == x &&
                    game->current.y + current_blocks[b].y == board_y) {
                  is_active = true;
                  break;
                }
              }
            }

            // 2. Is ghost piece block?
            bool is_ghost = false;
            if (!is_active && (game->state == STATE_PLAYING || game->state == STATE_PAUSED)) {
              for (int b = 0; b < 4; b++) {
                if (game->current.x + current_blocks[b].x == x &&
                    ghost_y + current_blocks[b].y == board_y) {
                  is_ghost = true;
                  break;
                }
              }
            }

            if (is_active) {
              row_offset += snprintf(playfield_part + row_offset, sizeof(playfield_part) - row_offset, "%s██%s", PIECE_COLORS[game->current.type], COLOR_RESET);
            } else if (is_ghost) {
              row_offset += snprintf(playfield_part + row_offset, sizeof(playfield_part) - row_offset, "%s[]%s", COLOR_GHOST, COLOR_RESET);
            } else if (game->board[board_y][x] != TET_NONE) {
              row_offset += snprintf(playfield_part + row_offset, sizeof(playfield_part) - row_offset, "%s██%s", PIECE_COLORS[game->board[board_y][x]], COLOR_RESET);
            } else {
              row_offset += snprintf(playfield_part + row_offset, sizeof(playfield_part) - row_offset, "%s ·%s", COLOR_GRID, COLOR_RESET);
            }
          }
        }

        row_offset += snprintf(playfield_part + row_offset, sizeof(playfield_part) - row_offset, "%s│%s", COLOR_BORDER, COLOR_RESET);
      }
    }

    // 3. RIGHT PANEL (Next queue & Controls)
    char right_part[256];
    switch (line) {
      case 0:
        snprintf(right_part, sizeof(right_part), "    %s┌── NEXT ──┐%s", COLOR_BORDER, COLOR_RESET);
        break;
      case 1:
        snprintf(right_part, sizeof(right_part), "    %s│%s %s %s│%s", COLOR_BORDER, COLOR_RESET, next0_r0, COLOR_BORDER, COLOR_RESET);
        break;
      case 2:
        snprintf(right_part, sizeof(right_part), "    %s│%s %s %s│%s", COLOR_BORDER, COLOR_RESET, next0_r1, COLOR_BORDER, COLOR_RESET);
        break;
      case 3:
        snprintf(right_part, sizeof(right_part), "    %s│%s ──────── %s│%s", COLOR_BORDER, COLOR_RESET, COLOR_BORDER, COLOR_RESET);
        break;
      case 4:
        snprintf(right_part, sizeof(right_part), "    %s│%s %s %s│%s", COLOR_BORDER, COLOR_RESET, next1_r0, COLOR_BORDER, COLOR_RESET);
        break;
      case 5:
        snprintf(right_part, sizeof(right_part), "    %s│%s %s %s│%s", COLOR_BORDER, COLOR_RESET, next1_r1, COLOR_BORDER, COLOR_RESET);
        break;
      case 6:
        snprintf(right_part, sizeof(right_part), "    %s│%s ──────── %s│%s", COLOR_BORDER, COLOR_RESET, COLOR_BORDER, COLOR_RESET);
        break;
      case 7:
        snprintf(right_part, sizeof(right_part), "    %s│%s %s %s│%s", COLOR_BORDER, COLOR_RESET, next2_r0, COLOR_BORDER, COLOR_RESET);
        break;
      case 8:
        snprintf(right_part, sizeof(right_part), "    %s│%s %s %s│%s", COLOR_BORDER, COLOR_RESET, next2_r1, COLOR_BORDER, COLOR_RESET);
        break;
      case 9:
        snprintf(right_part, sizeof(right_part), "    %s└──────────┘%s", COLOR_BORDER, COLOR_RESET);
        break;
      case 10:
        snprintf(right_part, sizeof(right_part), "    %s┌─ CONTROLS ─┐%s", COLOR_BORDER, COLOR_RESET);
        break;
      case 11:
        snprintf(right_part, sizeof(right_part), "    %s│%s ←/→: Move  %s│%s", COLOR_BORDER, COLOR_RESET, COLOR_BORDER, COLOR_RESET);
        break;
      case 12:
        snprintf(right_part, sizeof(right_part), "    %s│%s ↑/W: Rot CW%s│%s", COLOR_BORDER, COLOR_RESET, COLOR_BORDER, COLOR_RESET);
        break;
      case 13:
        snprintf(right_part, sizeof(right_part), "    %s│%s Z  : Rot CC%s│%s", COLOR_BORDER, COLOR_RESET, COLOR_BORDER, COLOR_RESET);
        break;
      case 14:
        snprintf(right_part, sizeof(right_part), "    %s│%s ↓/S: Soft   %s│%s", COLOR_BORDER, COLOR_RESET, COLOR_BORDER, COLOR_RESET);
        break;
      case 15:
        snprintf(right_part, sizeof(right_part), "    %s│%s Spc: Hard   %s│%s", COLOR_BORDER, COLOR_RESET, COLOR_BORDER, COLOR_RESET);
        break;
      case 16:
        snprintf(right_part, sizeof(right_part), "    %s│%s C/H: Hold   %s│%s", COLOR_BORDER, COLOR_RESET, COLOR_BORDER, COLOR_RESET);
        break;
      case 17:
        snprintf(right_part, sizeof(right_part), "    %s│%s P  : Pause  %s│%s", COLOR_BORDER, COLOR_RESET, COLOR_BORDER, COLOR_RESET);
        break;
      case 18:
        snprintf(right_part, sizeof(right_part), "    %s│%s R  : Reset  %s│%s", COLOR_BORDER, COLOR_RESET, COLOR_BORDER, COLOR_RESET);
        break;
      case 19:
        snprintf(right_part, sizeof(right_part), "    %s│%s Q  : Quit   %s│%s", COLOR_BORDER, COLOR_RESET, COLOR_BORDER, COLOR_RESET);
        break;
      case 20:
        snprintf(right_part, sizeof(right_part), "    %s└────────────┘%s", COLOR_BORDER, COLOR_RESET);
        break;
      default:
        snprintf(right_part, sizeof(right_part), "                  ");
        break;
    }

    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s%s%s\n", left_part, playfield_part, right_part);
  }

  // Footer status bar
  offset += snprintf(
    buffer + offset, sizeof(buffer) - offset,
    "\n  %s[ Antigravity Terminal Edition • 60 FPS • Guideline SRS ]%s\n",
    COLOR_HINT, COLOR_RESET
  );

  fwrite(buffer, 1, offset, stdout);
  fflush(stdout);
}
