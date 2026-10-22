#ifndef LIFE_GAME_H
#define LIFE_GAME_H

#include "config.h"

typedef struct {
  unsigned char grid[GRID_ROWS][GRID_COLS];       /* 0 = dead, 1..255 = age */
  unsigned char next_grid[GRID_ROWS][GRID_COLS];  /* Scratch buffer */

  unsigned char history[MAX_HISTORY][GRID_ROWS][GRID_COLS];
  int history_count;

  int cursor_r;                                   /* 0..GRID_ROWS-1 */
  int cursor_c;                                   /* 0..GRID_COLS-1 */

  bool is_running;                                /* Simulation running vs paused */
  unsigned long long generation;                  /* Current generation count */
  int population;                                 /* Current living cell count */
  int peak_population;                            /* Peak living cell count this session */

  int selected_preset;                            /* 0..MAX_PRESETS-1 */
  WrapMode wrap_mode;                             /* WRAP_TOROIDAL or WRAP_BOUNDED */
  int speed_level;                                /* 1..10 */
  double sim_timer;                               /* Step timer accumulator */

  int lifetime_generations;                       /* Loaded & saved in scores.ini */
  int lifetime_max_pop;                           /* Loaded & saved in scores.ini */

  char status_msg[96];                            /* Bottom status line */
} GameState;

void game_init(GameState *g);
void game_clear(GameState *g);
void game_randomize(GameState *g, int density_percent);

void game_toggle_cell(GameState *g, int r, int c);
void game_set_cell(GameState *g, int r, int c, bool alive);

void game_step_forward(GameState *g);
bool game_step_backward(GameState *g);
void game_toggle_pause(GameState *g);

void game_cycle_preset(GameState *g);
void game_stamp_preset(GameState *g, int top_r, int left_c, int preset_idx);
void game_toggle_wrap(GameState *g);
void game_adjust_speed(GameState *g, int delta);
void game_move_cursor(GameState *g, int dr, int dc);

void game_update(GameState *g, double dt);
void game_save_scores(const GameState *g);

int game_count_neighbors(const GameState *g, int r, int c);

#endif /* LIFE_GAME_H */
