#include "game.h"
#include "scores.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

static void recalculate_population(GameState *g) {
  int count = 0;
  for (int r = 0; r < GRID_ROWS; r++) {
    for (int c = 0; c < GRID_COLS; c++) {
      if (g->grid[r][c] > 0) {
        count++;
      }
    }
  }
  g->population = count;
  if (count > g->peak_population) {
    g->peak_population = count;
  }
  if (count > g->lifetime_max_pop) {
    g->lifetime_max_pop = count;
  }
}

int game_count_neighbors(const GameState *g, int r, int c) {
  int count = 0;
  for (int dr = -1; dr <= 1; dr++) {
    for (int dc = -1; dc <= 1; dc++) {
      if (dr == 0 && dc == 0) continue;
      int nr = r + dr;
      int nc = c + dc;

      if (g->wrap_mode == WRAP_TOROIDAL) {
        nr = (nr % GRID_ROWS + GRID_ROWS) % GRID_ROWS;
        nc = (nc % GRID_COLS + GRID_COLS) % GRID_COLS;
      } else {
        if (nr < 0 || nr >= GRID_ROWS || nc < 0 || nc >= GRID_COLS) {
          continue;
        }
      }

      if (g->grid[nr][nc] > 0) {
        count++;
      }
    }
  }
  return count;
}

void game_clear(GameState *g) {
  memset(g->grid, 0, sizeof(g->grid));
  g->history_count = 0;
  g->generation = 0;
  g->population = 0;
  g->peak_population = 0;
  g->is_running = false;
  g->sim_timer = 0.0;
  snprintf(g->status_msg, sizeof(g->status_msg),
           "Grid cleared. [Click / Enter] Draw  [S] Stamp  [Space] Run");
}

void game_randomize(GameState *g, int density_percent) {
  if (density_percent < 5) density_percent = 5;
  if (density_percent > 90) density_percent = 90;

  for (int r = 0; r < GRID_ROWS; r++) {
    for (int c = 0; c < GRID_COLS; c++) {
      g->grid[r][c] = ((rand() % 100) < density_percent) ? 1 : 0;
    }
  }
  g->history_count = 0;
  g->generation = 0;
  g->peak_population = 0;
  recalculate_population(g);

  snprintf(g->status_msg, sizeof(g->status_msg),
           "Random seed (%d%% density, %d live cells). Press [Space] to run.",
           density_percent, g->population);
}

void game_toggle_cell(GameState *g, int r, int c) {
  if (r < 0 || r >= GRID_ROWS || c < 0 || c >= GRID_COLS) return;
  g->grid[r][c] = (g->grid[r][c] > 0) ? 0 : 1;
  recalculate_population(g);
  snprintf(g->status_msg, sizeof(g->status_msg),
           "Toggled cell at (%d, %d) -> %s", r, c, (g->grid[r][c] > 0) ? "ALIVE" : "DEAD");
}

void game_set_cell(GameState *g, int r, int c, bool alive) {
  if (r < 0 || r >= GRID_ROWS || c < 0 || c >= GRID_COLS) return;
  g->grid[r][c] = alive ? 1 : 0;
  recalculate_population(g);
}

void game_step_forward(GameState *g) {
  /* Push to history */
  if (g->history_count >= MAX_HISTORY) {
    memmove(&g->history[0], &g->history[1], sizeof(g->grid) * (MAX_HISTORY - 1));
    g->history_count = MAX_HISTORY - 1;
  }
  memcpy(g->history[g->history_count++], g->grid, sizeof(g->grid));

  /* Conway B3/S23 simulation step */
  for (int r = 0; r < GRID_ROWS; r++) {
    for (int c = 0; c < GRID_COLS; c++) {
      int neighbors = game_count_neighbors(g, r, c);
      unsigned char age = g->grid[r][c];

      if (age > 0) {
        /* Survival: 2 or 3 live neighbors */
        if (neighbors == 2 || neighbors == 3) {
          g->next_grid[r][c] = (age < 255) ? (age + 1) : 255;
        } else {
          g->next_grid[r][c] = 0; /* Underpopulation or Overpopulation */
        }
      } else {
        /* Reproduction: exactly 3 live neighbors */
        if (neighbors == 3) {
          g->next_grid[r][c] = 1;
        } else {
          g->next_grid[r][c] = 0;
        }
      }
    }
  }

  memcpy(g->grid, g->next_grid, sizeof(g->grid));
  g->generation++;
  g->lifetime_generations++;
  recalculate_population(g);
}

bool game_step_backward(GameState *g) {
  if (g->history_count <= 0) {
    snprintf(g->status_msg, sizeof(g->status_msg), "Cannot step back (history empty).");
    return false;
  }

  g->history_count--;
  memcpy(g->grid, g->history[g->history_count], sizeof(g->grid));
  if (g->generation > 0) {
    g->generation--;
  }
  recalculate_population(g);
  snprintf(g->status_msg, sizeof(g->status_msg),
           "Stepped back to Gen %llu (%d live cells).", g->generation, g->population);
  return true;
}

void game_toggle_pause(GameState *g) {
  g->is_running = !g->is_running;
  snprintf(g->status_msg, sizeof(g->status_msg),
           g->is_running ? "▶ Simulation running..." : "⏸ Simulation paused.");
}

void game_cycle_preset(GameState *g) {
  g->selected_preset = (g->selected_preset + 1) % MAX_PRESETS;
  snprintf(g->status_msg, sizeof(g->status_msg),
           "Selected Preset [%d/%d]: %s (%s)",
           g->selected_preset + 1, MAX_PRESETS,
           LIFE_PRESETS[g->selected_preset].name,
           LIFE_PRESETS[g->selected_preset].category);
}

void game_stamp_preset(GameState *g, int top_r, int left_c, int preset_idx) {
  if (preset_idx < 0 || preset_idx >= MAX_PRESETS) preset_idx = 0;
  const LifePreset *p = &LIFE_PRESETS[preset_idx];

  for (int pr = 0; pr < p->height; pr++) {
    for (int pc = 0; pc < p->width; pc++) {
      char ch = p->pattern[pr][pc];
      if (ch == 'O' || ch == '#' || ch == '1') {
        int r = top_r + pr;
        int c = left_c + pc;
        if (g->wrap_mode == WRAP_TOROIDAL) {
          r = (r % GRID_ROWS + GRID_ROWS) % GRID_ROWS;
          c = (c % GRID_COLS + GRID_COLS) % GRID_COLS;
        }
        if (r >= 0 && r < GRID_ROWS && c >= 0 && c < GRID_COLS) {
          g->grid[r][c] = 1;
        }
      }
    }
  }

  recalculate_population(g);
  snprintf(g->status_msg, sizeof(g->status_msg),
           "Stamped '%s' at (%d, %d). Total Pop: %d",
           p->name, top_r, left_c, g->population);
}

void game_toggle_wrap(GameState *g) {
  g->wrap_mode = (g->wrap_mode == WRAP_TOROIDAL) ? WRAP_BOUNDED : WRAP_TOROIDAL;
  snprintf(g->status_msg, sizeof(g->status_msg),
           "Boundary Mode: %s", (g->wrap_mode == WRAP_TOROIDAL) ? "TOROIDAL (Wrap)" : "BOUNDED (Walls)");
}

void game_adjust_speed(GameState *g, int delta) {
  g->speed_level += delta;
  if (g->speed_level < 1) g->speed_level = 1;
  if (g->speed_level > 10) g->speed_level = 10;
  snprintf(g->status_msg, sizeof(g->status_msg),
           "Speed adjusted: Level %d / 10", g->speed_level);
}

void game_move_cursor(GameState *g, int dr, int dc) {
  int nr = g->cursor_r + dr;
  int nc = g->cursor_c + dc;
  if (nr >= 0 && nr < GRID_ROWS) g->cursor_r = nr;
  if (nc >= 0 && nc < GRID_COLS) g->cursor_c = nc;
}

void game_update(GameState *g, double dt) {
  if (!g->is_running) return;

  /* Speed scale: Level 1 = 2.0s, Level 5 = 0.12s, Level 10 = 0.03s */
  static const double speed_intervals[10] = {
    1.00, 0.50, 0.25, 0.16, 0.10, 0.07, 0.05, 0.035, 0.025, 0.016
  };
  double interval = speed_intervals[g->speed_level - 1];

  g->sim_timer += dt;
  while (g->sim_timer >= interval) {
    game_step_forward(g);
    g->sim_timer -= interval;
  }
}

void game_init(GameState *g) {
  memset(g, 0, sizeof(GameState));
  srand((unsigned int)time(NULL));

  g->wrap_mode = WRAP_TOROIDAL;
  g->speed_level = 5; /* 10 steps per second */
  g->cursor_r = GRID_ROWS / 2;
  g->cursor_c = GRID_COLS / 2;
  g->selected_preset = 0; /* Glider */
  g->is_running = false;

  g->lifetime_max_pop = arcade_score_get("life", "max_population");
  g->lifetime_generations = arcade_score_get("life", "total_generations");

  /* Default starter pattern: Pulsar at center */
  game_stamp_preset(g, 3, 8, 5); /* Pulsar */
  g->generation = 0;

  snprintf(g->status_msg, sizeof(g->status_msg),
           "Welcome to Conway's Game of Life! Press [Space] to run / pause.");
}

void game_save_scores(const GameState *g) {
  arcade_score_set("life", "max_population", g->lifetime_max_pop);
  arcade_score_set("life", "total_generations", g->lifetime_generations);
}
