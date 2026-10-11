#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../../include/scores.h"

void game_load_scores(GameState *game) {
  game->best_times[0] = arcade_score_get("minesweeper", "best_time_beginner");
  game->best_times[1] = arcade_score_get("minesweeper", "best_time_intermediate");
  game->best_times[2] = arcade_score_get("minesweeper", "best_time_expert");
}

void game_save_scores(const GameState *game) {
  arcade_score_set("minesweeper", "best_time_beginner", game->best_times[0]);
  arcade_score_set("minesweeper", "best_time_intermediate", game->best_times[1]);
  arcade_score_set("minesweeper", "best_time_expert", game->best_times[2]);
}

void game_set_difficulty(GameState *game, Difficulty diff) {
  game->difficulty = diff;
  switch (diff) {
    case DIFF_BEGINNER:
      game->rows = 9;
      game->cols = 9;
      game->total_mines = 10;
      break;

    case DIFF_INTERMEDIATE:
      game->rows = 16;
      game->cols = 16;
      game->total_mines = 40;
      break;

    case DIFF_EXPERT:
      game->rows = 16;
      game->cols = 30;
      game->total_mines = 99;
      break;

    default:
      game->rows = 9;
      game->cols = 9;
      game->total_mines = 10;
      break;
  }
  game_reset(game);
}

void game_reset(GameState *game) {
  memset(game->grid, 0, sizeof(game->grid));
  game->flags_placed = 0;
  game->revealed_count = 0;
  game->cursor_r = game->rows / 2;
  game->cursor_c = game->cols / 2;
  game->state = STATE_READY;
  game->start_time = 0;
  game->end_time = 0;
  game->hit_mine_r = -1;
  game->hit_mine_c = -1;
}

void game_init(GameState *game) {
  game_load_scores(game);
  game_set_difficulty(game, DIFF_BEGINNER);
}

void game_move_cursor(GameState *game, int dr, int dc) {
  int new_r = game->cursor_r + dr;
  int new_c = game->cursor_c + dc;
  if (new_r >= 0 && new_r < game->rows) {
    game->cursor_r = new_r;
  }
  if (new_c >= 0 && new_c < game->cols) {
    game->cursor_c = new_c;
  }
}

static void generate_mines(GameState *game, int safe_r, int safe_c) {
  int placed = 0;
  while (placed < game->total_mines) {
    int r = rand() % game->rows;
    int c = rand() % game->cols;

    // First click safe zone: don't place in clicked cell or adjacent if possible
    bool is_safe = (abs(r - safe_r) <= 1 && abs(c - safe_c) <= 1);
    if (is_safe && (game->rows * game->cols - 9 >= game->total_mines)) {
      continue;
    }
    if (r == safe_r && c == safe_c) {
      continue;
    }

    if (!game->grid[r][c].is_mine) {
      game->grid[r][c].is_mine = true;
      placed++;
    }
  }

  // Count neighbor mines
  for (int r = 0; r < game->rows; r++) {
    for (int c = 0; c < game->cols; c++) {
      if (game->grid[r][c].is_mine) {
        continue;
      }
      int count = 0;
      for (int dr = -1; dr <= 1; dr++) {
        for (int dc = -1; dc <= 1; dc++) {
          int nr = r + dr;
          int nc = c + dc;
          if (nr >= 0 && nr < game->rows && nc >= 0 && nc < game->cols) {
            if (game->grid[nr][nc].is_mine) {
              count++;
            }
          }
        }
      }
      game->grid[r][c].neighbor_mines = count;
    }
  }
}

static void reveal_recursive(GameState *game, int r, int c) {
  if (r < 0 || r >= game->rows || c < 0 || c >= game->cols) {
    return;
  }
  Cell *cell = &game->grid[r][c];
  if (cell->is_revealed || cell->is_flagged || cell->is_mine) {
    return;
  }

  cell->is_revealed = true;
  game->revealed_count++;

  if (cell->neighbor_mines == 0) {
    for (int dr = -1; dr <= 1; dr++) {
      for (int dc = -1; dc <= 1; dc++) {
        if (dr != 0 || dc != 0) {
          reveal_recursive(game, r + dr, c + dc);
        }
      }
    }
  }
}

static void check_win_condition(GameState *game, unsigned long long current_time) {
  int target_reveals = game->rows * game->cols - game->total_mines;
  if (game->revealed_count == target_reveals) {
    game->state = STATE_WON;
    game->end_time = current_time;

    // Auto-flag all mines
    for (int r = 0; r < game->rows; r++) {
      for (int c = 0; c < game->cols; c++) {
        if (game->grid[r][c].is_mine) {
          game->grid[r][c].is_flagged = true;
        }
      }
    }
    game->flags_placed = game->total_mines;

    int elapsed = game_get_elapsed_seconds(game, current_time);
    int current_best = game->best_times[game->difficulty];
    if (current_best == 0 || elapsed < current_best) {
      game->best_times[game->difficulty] = elapsed;
      game_save_scores(game);
    }
  }
}

void game_reveal(GameState *game, int r, int c, unsigned long long current_time) {
  if (r < 0 || r >= game->rows || c < 0 || c >= game->cols) {
    return;
  }
  if (game->state != STATE_READY && game->state != STATE_PLAYING) {
    return;
  }
  if (game->grid[r][c].is_flagged || game->grid[r][c].is_revealed) {
    return;
  }

  if (game->state == STATE_READY) {
    game->state = STATE_PLAYING;
    game->start_time = current_time;
    generate_mines(game, r, c);
  }

  if (game->grid[r][c].is_mine) {
    // Game Over: hit a mine!
    game->state = STATE_GAME_OVER;
    game->end_time = current_time;
    game->hit_mine_r = r;
    game->hit_mine_c = c;

    // Reveal all mines
    for (int gr = 0; gr < game->rows; gr++) {
      for (int gc = 0; gc < game->cols; gc++) {
        if (game->grid[gr][gc].is_mine) {
          game->grid[gr][gc].is_revealed = true;
        }
      }
    }
    return;
  }

  reveal_recursive(game, r, c);
  check_win_condition(game, current_time);
}

void game_toggle_flag(GameState *game, int r, int c) {
  if (r < 0 || r >= game->rows || c < 0 || c >= game->cols) {
    return;
  }
  if (game->state != STATE_READY && game->state != STATE_PLAYING) {
    return;
  }
  if (game->grid[r][c].is_revealed) {
    return;
  }

  if (game->grid[r][c].is_flagged) {
    game->grid[r][c].is_flagged = false;
    game->flags_placed--;
  } else {
    game->grid[r][c].is_flagged = true;
    game->flags_placed++;
  }
}

void game_chord(GameState *game, int r, int c, unsigned long long current_time) {
  if (r < 0 || r >= game->rows || c < 0 || c >= game->cols) {
    return;
  }
  if (game->state != STATE_PLAYING) {
    return;
  }
  Cell *cell = &game->grid[r][c];
  if (!cell->is_revealed || cell->neighbor_mines == 0) {
    return;
  }

  // Count flags around this cell
  int flag_count = 0;
  for (int dr = -1; dr <= 1; dr++) {
    for (int dc = -1; dc <= 1; dc++) {
      int nr = r + dr;
      int nc = c + dc;
      if (nr >= 0 && nr < game->rows && nc >= 0 && nc < game->cols) {
        if (game->grid[nr][nc].is_flagged) {
          flag_count++;
        }
      }
    }
  }

  // If flags match neighbor mines, reveal unflagged neighbors
  if (flag_count == cell->neighbor_mines) {
    for (int dr = -1; dr <= 1; dr++) {
      for (int dc = -1; dc <= 1; dc++) {
        int nr = r + dr;
        int nc = c + dc;
        if (nr >= 0 && nr < game->rows && nc >= 0 && nc < game->cols) {
          if (!game->grid[nr][nc].is_flagged && !game->grid[nr][nc].is_revealed) {
            game_reveal(game, nr, nc, current_time);
            if (game->state == STATE_GAME_OVER) {
              return;
            }
          }
        }
      }
    }
  }
}

int game_get_elapsed_seconds(const GameState *game, unsigned long long current_time) {
  if (game->state == STATE_READY) {
    return 0;
  }
  unsigned long long end = (game->end_time > 0) ? game->end_time : current_time;
  if (end < game->start_time) {
    return 0;
  }
  return (int)((end - game->start_time) / 1000ULL);
}
