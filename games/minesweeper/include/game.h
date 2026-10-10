#ifndef GAME_MINESWEEPER_H
#define GAME_MINESWEEPER_H

#include "config.h"

typedef struct {
  bool is_mine;
  bool is_revealed;
  bool is_flagged;
  int neighbor_mines;
} Cell;

typedef struct {
  Cell grid[MAX_ROWS][MAX_COLS];
  int rows;
  int cols;
  int total_mines;
  int flags_placed;
  int revealed_count;

  int cursor_r;
  int cursor_c;

  Difficulty difficulty;
  PlayState state;
  unsigned long long start_time;
  unsigned long long end_time;
  int best_times[3]; // Beginner, Intermediate, Expert in seconds (0 = unranked)

  int hit_mine_r;
  int hit_mine_c;
} GameState;

void game_init(GameState *game);
void game_set_difficulty(GameState *game, Difficulty diff);
void game_reset(GameState *game);
void game_move_cursor(GameState *game, int dr, int dc);
void game_reveal(GameState *game, int r, int c, unsigned long long current_time);
void game_toggle_flag(GameState *game, int r, int c);
void game_chord(GameState *game, int r, int c, unsigned long long current_time);
int game_get_elapsed_seconds(const GameState *game, unsigned long long current_time);
void game_load_scores(GameState *game);
void game_save_scores(const GameState *game);

#endif /* GAME_MINESWEEPER_H */
