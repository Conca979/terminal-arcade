#include <conio.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include "scores.h"

#define COLOR_RESET "\033[0m"
#define COLOR_BORDER "\033[38;2;80;120;175m"
#define COLOR_TITLE "\033[1;38;2;255;255;255m"
#define COLOR_SEL "\033[1;38;2;255;220;50;48;2;45;65;95m"
#define COLOR_DIM "\033[38;2;140;150;170m"
#define COLOR_ACCENT "\033[1;38;2;0;220;210m"
#define COLOR_SCORE "\033[1;38;2;255;215;60m"

typedef struct {
  const char *title;
  const char *desc;
  const char *executable_path;
  const char *score_section;
  const char *score_key;
} GameEntry;

static const GameEntry GAMES[] = {
  {
    "Tetris CLI",
    "Classic Guideline Tetris with SRS & 7-bag",
    "games\\tetris\\tetris.exe",
    "tetris",
    "high_score"
  },
  {
    "2048 CLI",
    "4x4 slide & merge puzzle with 32-step undo",
    "games\\2048\\2048.exe",
    "2048",
    "high_score"
  },
  {
    "Minesweeper CLI",
    "Minefield sweeper with first-click safety",
    "games\\minesweeper\\minesweeper.exe",
    "minesweeper",
    "best_time_beginner"
  },
  {
    "Cyberpunk Snake",
    "Neon light-cycle snake with power-ups",
    "games\\snake\\snake.exe",
    "snake",
    "high_score"
  },
  {
    "Space Invaders CLI",
    "Arcade shmup with marching swarms & UFO",
    "games\\invaders\\invaders.exe",
    "invaders",
    "high_score"
  },
  {
    "Sudoku CLI",
    "Logic number grid with notes & inspector",
    "games\\sudoku\\sudoku.exe",
    NULL,
    NULL
  },
  {
    "Bricks Breaker CLI",
    "Endless physics ball swarm with mouse aim",
    "games\\breaker\\breaker.exe",
    "breaker",
    "high_score"
  },
  {
    "Block Puzzle CLI",
    "Classic 10x10 wood puzzle with mouse & combo",
    "games\\block\\block.exe",
    "block",
    "high_score"
  },
  {
    "Classic Pong CLI",
    "Player vs CPU with retro scoreboard & AI",
    "games\\pong\\pong.exe",
    "pong",
    "high_score"
  },
  {
    "Typing Defense CLI",
    "Cyber citadel typing shmup with 5,800+ words",
    "games\\typing\\typing.exe",
    "typing",
    "high_score"
  },
  {
    "Wordle CLI",
    "Classic 5-letter word puzzle with live stats",
    "games\\wordle\\wordle.exe",
    "wordle",
    "games_won"
  }
};

static const int GAME_COUNT = sizeof(GAMES) / sizeof(GAMES[0]);

static void setup_console(void) {
  HANDLE h_out = GetStdHandle(STD_OUTPUT_HANDLE);
  if (h_out != INVALID_HANDLE_VALUE) {
    DWORD mode = 0;
    GetConsoleMode(h_out, &mode);
    SetConsoleMode(h_out, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING | ENABLE_PROCESSED_OUTPUT);
  }
  SetConsoleOutputCP(CP_UTF8);
}

static void draw_menu(int selected) {
  printf("\033[2J\033[H");

  printf(
    "  %s╔═════════════════════════════════════════════════════════════════════════════╗%s\n"
    "  %s║%s                      %sT E R M I N A L   A R C A D E   ( C )%s                      %s║%s\n"
    "  %s╚═════════════════════════════════════════════════════════════════════════════╝%s\n\n",
    COLOR_BORDER, COLOR_RESET,
    COLOR_BORDER, COLOR_RESET, COLOR_TITLE, COLOR_RESET, COLOR_BORDER, COLOR_RESET,
    COLOR_BORDER, COLOR_RESET
  );

  printf("  %sChoose a game to play:%s\n\n", COLOR_ACCENT, COLOR_RESET);

  for (int i = 0; i < GAME_COUNT; i++) {
    char score_badge[64] = "";
    if (GAMES[i].score_section && GAMES[i].score_key) {
      int score = arcade_score_get(GAMES[i].score_section, GAMES[i].score_key);
      if (score > 0) {
        if (strcmp(GAMES[i].score_section, "minesweeper") == 0) {
          snprintf(score_badge, sizeof(score_badge), "[Best: %ds]", score);
        } else if (strcmp(GAMES[i].score_section, "pong") == 0) {
          snprintf(score_badge, sizeof(score_badge), "[Rally: %d]", score);
        } else if (strcmp(GAMES[i].score_section, "typing") == 0) {
          int wpm = arcade_score_get("typing", "max_wpm");
          if (wpm > 0) {
            snprintf(score_badge, sizeof(score_badge), "[%d | %dWPM]", score, wpm);
          } else {
            snprintf(score_badge, sizeof(score_badge), "[Best: %d]", score);
          }
        } else if (strcmp(GAMES[i].score_section, "wordle") == 0) {
          int streak = arcade_score_get("wordle", "win_streak");
          if (streak > 0) {
            snprintf(score_badge, sizeof(score_badge), "[Won: %d|🔥%d]", score, streak);
          } else {
            snprintf(score_badge, sizeof(score_badge), "[Won: %d]", score);
          }
        } else {
          snprintf(score_badge, sizeof(score_badge), "[Best: %d]", score);
        }
      }
    }

    if (i == selected) {
      printf("    %s > [%d] %-18s %-16s %-44s %s\n",
             COLOR_SEL, i + 1, GAMES[i].title, score_badge, GAMES[i].desc, COLOR_RESET);
    } else {
      printf("      [%d] \033[1;37m%-18s\033[0m %s%-16s%s %s%-44s%s\n",
             i + 1, GAMES[i].title,
             COLOR_SCORE, score_badge, COLOR_RESET,
             COLOR_DIM, GAMES[i].desc, COLOR_RESET);
    }
  }

  printf("\n      [Q] \033[1;37mQuit Arcade\033[0m\n\n");
  printf("  %sControls:%s\n", COLOR_BORDER, COLOR_RESET);
  printf("    %s[↑ / ↓ / W / S]%s Navigate    %s[Enter / Space]%s Launch Game\n", COLOR_DIM, COLOR_RESET, COLOR_DIM, COLOR_RESET);
  printf("    %s[1 - %d]%s        Quick Select %s[Q]%s             Exit\n", COLOR_DIM, GAME_COUNT, COLOR_RESET, COLOR_DIM, COLOR_RESET);
  fflush(stdout);
}

static void launch_game(const char *exe_path) {
  // Check if executable exists
  FILE *f = fopen(exe_path, "rb");
  if (!f) {
    printf("\n  \033[1;31mGame not built yet (%s). Build with 'make' first!\033[0m\n", exe_path);
    printf("  Press any key to return...");
    fflush(stdout);
    _getch();
    return;
  }
  fclose(f);

  system(exe_path);
}

int main(void) {
  setup_console();

  int selected = 0;

  while (true) {
    draw_menu(selected);

    int ch = _getch();

    if (ch == 0 || ch == 224) {
      int ext = _getch();
      if (ext == 72) { // Up
        selected = (selected - 1 + GAME_COUNT) % GAME_COUNT;
      } else if (ext == 80) { // Down
        selected = (selected + 1) % GAME_COUNT;
      }
      continue;
    }

    if (ch == 'w' || ch == 'W') {
      selected = (selected - 1 + GAME_COUNT) % GAME_COUNT;
    } else if (ch == 's' || ch == 'S') {
      selected = (selected + 1) % GAME_COUNT;
    } else if (ch >= '1' && ch < '1' + GAME_COUNT) {
      selected = ch - '1';
      launch_game(GAMES[selected].executable_path);
    } else if (ch == '\r' || ch == '\n' || ch == ' ') {
      launch_game(GAMES[selected].executable_path);
    } else if (ch == 'q' || ch == 'Q' || ch == 27) {
      break;
    }
  }

  printf("\033[2J\033[H\033[0m");
  printf("Thanks for playing Terminal Arcade!\n");
  fflush(stdout);

  return 0;
}
