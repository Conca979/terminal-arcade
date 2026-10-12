#include "render.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#define COLOR_RESET "\033[0m"
#define COLOR_TITLE "\033[1;38;2;255;255;255m"
#define COLOR_BORDER "\033[38;2;90;110;140m"
#define COLOR_GRID "\033[38;2;75;85;105m"

// Official Wordle TrueColors
#define COLOR_CORRECT "\033[1;38;2;255;255;255;48;2;83;141;78m"  // #538d4e Emerald Green
#define COLOR_PRESENT "\033[1;38;2;255;255;255;48;2;181;159;59m" // #b59f3b Amber Yellow
#define COLOR_ABSENT  "\033[1;38;2;220;220;220;48;2;58;58;60m"   // #3a3a3c Slate Gray
#define COLOR_TYPING      "\033[1;38;2;255;255;255;48;2;55;70;95m"   // Active typed letter (crisp white on slate blue)
#define COLOR_ACTIVE_CELL "\033[1;38;2;0;225;215;48;2;30;42;58m"    // Active tile cursor focus
#define COLOR_EMPTY       "\033[48;2;18;22;32m"                      // Empty cell bg

#define COLOR_KEY_UNUSED  "\033[1;38;2;210;220;235;48;2;40;48;65m"
#define COLOR_KEY_ABSENT  "\033[1;38;2;140;150;165;48;2;30;32;38m"
#define COLOR_KEY_PRESENT "\033[1;38;2;255;255;255;48;2;181;159;59m"
#define COLOR_KEY_CORRECT "\033[1;38;2;255;255;255;48;2;83;141;78m"
#define COLOR_KEY_ENTER   "\033[1;38;2;255;255;255;48;2;45;75;110m"
#define COLOR_KEY_DEL     "\033[1;38;2;255;255;255;48;2;100;45;45m"

#define COLOR_BANNER_WIN  "\033[1;38;2;255;255;255;48;2;38;140;70m"
#define COLOR_BANNER_WARN "\033[1;38;2;255;240;120;48;2;160;50;40m"
#define COLOR_BANNER_INFO "\033[1;38;2;255;255;255;48;2;45;75;120m"

#define COLOR_LABEL "\033[38;2;140;160;190m"
#define COLOR_VAL   "\033[1;38;2;255;215;60m"
#define COLOR_BAR   "\033[38;2;83;141;78m"

void render_init(void) {
  printf("\033[2J\033[H");
  fflush(stdout);
}

void render_cleanup(void) {
  printf(COLOR_RESET);
  fflush(stdout);
}

static const char *get_key_color(KeyStatus ks) {
  switch (ks) {
    case KEY_CORRECT: return COLOR_KEY_CORRECT;
    case KEY_PRESENT: return COLOR_KEY_PRESENT;
    case KEY_ABSENT:  return COLOR_KEY_ABSENT;
    default:          return COLOR_KEY_UNUSED;
  }
}

void render_frame(const GameState *game) {
  static char buffer[32768];
  int offset = 0;

  offset += snprintf(buffer + offset, sizeof(buffer) - offset, "\033[H");

  // Master Title Banner (3 lines)
  offset += snprintf(
    buffer + offset, sizeof(buffer) - offset,
    "  %s╔════════════════════════════════════════════════════════════════════════════╗%s\n"
    "  %s║%s                        %sW  O  R  D  L  E   ( C )%s                       %s║%s\n"
    "  %s╚════════════════════════════════════════════════════════════════════════════╝%s\n",
    COLOR_BORDER, COLOR_RESET,
    COLOR_BORDER, COLOR_RESET, COLOR_TITLE, COLOR_RESET, COLOR_BORDER, COLOR_RESET,
    COLOR_BORDER, COLOR_RESET
  );

  // Stats calculation
  int win_pct = (game->games_played > 0) ? (game->games_won * 100 / game->games_played) : 0;
  int max_dist = 1;
  for (int i = 0; i < MAX_ATTEMPTS; i++) {
    if (game->guess_distribution[i] > max_dist) {
      max_dist = game->guess_distribution[i];
    }
  }

  // 13 lines total for the side-by-side board and statistics panel:
  for (int line = 0; line < 13; line++) {
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "    ");

    // Left side: Wordle 5x6 Board
    if (line % 2 == 0) {
      // Grid Border line
      const char *left_char = (line == 0) ? "┌" : (line == 12) ? "└" : "├";
      const char *mid_char  = (line == 0) ? "┬" : (line == 12) ? "┴" : "┼";
      const char *right_char = (line == 0) ? "┐" : (line == 12) ? "┘" : "┤";

      offset += snprintf(
        buffer + offset, sizeof(buffer) - offset,
        "%s%s───%s───%s───%s───%s───%s%s",
        COLOR_GRID, left_char, mid_char, mid_char, mid_char, mid_char, right_char, COLOR_RESET
      );
    } else {
      // Cell Letter line (board row = line / 2)
      int row_idx = line / 2;
      const GuessRow *row = &game->guesses[row_idx];

      offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s│%s", COLOR_GRID, COLOR_RESET);
      for (int c = 0; c < WORD_LEN; c++) {
        char ch = row->letters[c];
        TileStatus ts = row->status[c];
        bool is_cursor = (row_idx == game->current_attempt &&
                          game->state == STATE_PLAYING &&
                          c == game->current_letter_idx);

        if (ts == TILE_CORRECT) {
          offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s %c %s%s│%s", COLOR_CORRECT, ch ? ch : ' ', COLOR_RESET, COLOR_GRID, COLOR_RESET);
        } else if (ts == TILE_PRESENT) {
          offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s %c %s%s│%s", COLOR_PRESENT, ch ? ch : ' ', COLOR_RESET, COLOR_GRID, COLOR_RESET);
        } else if (ts == TILE_ABSENT) {
          offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s %c %s%s│%s", COLOR_ABSENT, ch ? ch : ' ', COLOR_RESET, COLOR_GRID, COLOR_RESET);
        } else if (ts == TILE_TYPING) {
          offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s %c %s%s│%s", COLOR_TYPING, ch ? ch : ' ', COLOR_RESET, COLOR_GRID, COLOR_RESET);
        } else if (is_cursor) {
          offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s _ %s%s│%s", COLOR_ACTIVE_CELL, COLOR_RESET, COLOR_GRID, COLOR_RESET);
        } else {
          offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s   %s%s│%s", COLOR_EMPTY, COLOR_RESET, COLOR_GRID, COLOR_RESET);
        }
      }
    }

    // Gap between board and stats
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "        ");

    // Right side: Statistics Panel
    switch (line) {
      case 0:
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s╔═════════════ STATISTICS ═════════════╗%s", COLOR_BORDER, COLOR_RESET);
        break;
      case 1:
        offset += snprintf(
          buffer + offset, sizeof(buffer) - offset,
          "%s║%s  Played: %s%-3d%s     Win Rate: %s%-3d%%%s       %s║%s",
          COLOR_BORDER, COLOR_RESET, COLOR_VAL, game->games_played, COLOR_RESET,
          COLOR_VAL, win_pct, COLOR_RESET, COLOR_BORDER, COLOR_RESET
        );
        break;
      case 2:
        offset += snprintf(
          buffer + offset, sizeof(buffer) - offset,
          "%s║%s  Streak: %s%-3d%s     Max Streak: %s%-3d%s     %s║%s",
          COLOR_BORDER, COLOR_RESET, COLOR_VAL, game->current_streak, COLOR_RESET,
          COLOR_VAL, game->max_streak, COLOR_RESET, COLOR_BORDER, COLOR_RESET
        );
        break;
      case 3:
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s╠══════════ GUESS DISTRIBUTION ═════════╣%s", COLOR_BORDER, COLOR_RESET);
        break;
      case 4:
      case 5:
      case 6:
      case 7:
      case 8:
      case 9: {
        int dist_idx = line - 4;
        int count = game->guess_distribution[dist_idx];
        int bar_len = (max_dist > 0) ? (count * 18 / max_dist) : 0;
        offset += snprintf(
          buffer + offset, sizeof(buffer) - offset,
          "%s║%s  %d │ %s",
          COLOR_BORDER, COLOR_RESET, dist_idx + 1, COLOR_BAR
        );
        for (int b = 0; b < bar_len && b < 18; b++) {
          offset += snprintf(buffer + offset, sizeof(buffer) - offset, "█");
        }
        for (int b = bar_len; b < 18; b++) {
          offset += snprintf(buffer + offset, sizeof(buffer) - offset, " ");
        }
        offset += snprintf(
          buffer + offset, sizeof(buffer) - offset,
          "%s %s%-2d%s             %s║%s",
          COLOR_RESET, COLOR_VAL, count, COLOR_RESET, COLOR_BORDER, COLOR_RESET
        );
        break;
      }
      case 10:
        offset += snprintf(
          buffer + offset, sizeof(buffer) - offset,
          "%s║%s  Secret Word Length: 5 Letters        %s║%s",
          COLOR_BORDER, COLOR_RESET, COLOR_BORDER, COLOR_RESET
        );
        break;
      case 11:
        offset += snprintf(
          buffer + offset, sizeof(buffer) - offset,
          "%s║%s  Vocabulary: 14,800+ Guess Words      %s║%s",
          COLOR_BORDER, COLOR_RESET, COLOR_BORDER, COLOR_RESET
        );
        break;
      case 12:
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s╚═══════════════════════════════════════╝%s", COLOR_BORDER, COLOR_RESET);
        break;
      default:
        break;
    }

    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "\n");
  }

  // Notification / Message Banner below board (1 line)
  if (game->message[0] != '\0') {
    const char *banner_col = (game->state == STATE_WON) ? COLOR_BANNER_WIN :
                             (game->state == STATE_LOST) ? COLOR_BANNER_WARN : COLOR_BANNER_INFO;
    offset += snprintf(
      buffer + offset, sizeof(buffer) - offset,
      "      %s                >>> %-26s <<<                %s\n",
      banner_col, game->message, COLOR_RESET
    );
  } else {
    offset += snprintf(
      buffer + offset, sizeof(buffer) - offset,
      "      %s           Guess attempt %d of %d  │  Type a 5-letter word           %s\n",
      COLOR_LABEL, game->current_attempt + 1, MAX_ATTEMPTS, COLOR_RESET
    );
  }

  // QWERTY Virtual Keyboard (3 lines)
  // Row 1: Q W E R T Y U I O P
  static const char ROW1[] = "QWERTYUIOP";
  offset += snprintf(buffer + offset, sizeof(buffer) - offset, "       ");
  for (int i = 0; ROW1[i] != '\0'; i++) {
    char k = ROW1[i];
    KeyStatus ks = game->key_status[k - 'A'];
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s %c %s ", get_key_color(ks), k, COLOR_RESET);
  }
  offset += snprintf(buffer + offset, sizeof(buffer) - offset, "\n");

  // Row 2: A S D F G H J K L
  static const char ROW2[] = "ASDFGHJKL";
  offset += snprintf(buffer + offset, sizeof(buffer) - offset, "         ");
  for (int i = 0; ROW2[i] != '\0'; i++) {
    char k = ROW2[i];
    KeyStatus ks = game->key_status[k - 'A'];
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s %c %s ", get_key_color(ks), k, COLOR_RESET);
  }
  offset += snprintf(buffer + offset, sizeof(buffer) - offset, "\n");

  // Row 3: ENTER  Z X C V B N M  DEL
  static const char ROW3[] = "ZXCVBNM";
  offset += snprintf(buffer + offset, sizeof(buffer) - offset, "    ");
  offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s ENTER %s ", COLOR_KEY_ENTER, COLOR_RESET);
  for (int i = 0; ROW3[i] != '\0'; i++) {
    char k = ROW3[i];
    KeyStatus ks = game->key_status[k - 'A'];
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s %c %s ", get_key_color(ks), k, COLOR_RESET);
  }
  offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s DEL %s\n", COLOR_KEY_DEL, COLOR_RESET);

  // Footer Controls Reference (1 line) - Total 21 lines
  if (game->state == STATE_WON || game->state == STATE_LOST) {
    offset += snprintf(
      buffer + offset, sizeof(buffer) - offset,
      "  %sControls:%s %s[Enter / Space / R]%s Play Next Puzzle  │  %s[Q / Esc]%s Return to Arcade\n",
      COLOR_LABEL, COLOR_RESET, COLOR_TITLE, COLOR_RESET, COLOR_TITLE, COLOR_RESET
    );
  } else {
    offset += snprintf(
      buffer + offset, sizeof(buffer) - offset,
      "  %sControls:%s %s[A - Z]%s Letter  │  %s[Backspace]%s Del  │  %s[Enter]%s Submit  │  %s[Esc]%s Quit\n",
      COLOR_LABEL, COLOR_RESET, COLOR_TITLE, COLOR_RESET, COLOR_TITLE, COLOR_RESET, COLOR_TITLE, COLOR_RESET, COLOR_TITLE, COLOR_RESET
    );
  }

  fwrite(buffer, 1, offset, stdout);
  fflush(stdout);
}
