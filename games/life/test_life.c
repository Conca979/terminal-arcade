#include "config.h"
#include "game.h"
#include "render.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_blinker_oscillator(void) {
  GameState g;
  game_init(&g);
  game_clear(&g);

  /* Horizontal blinker at row 5: (5, 5), (5, 6), (5, 7) */
  game_set_cell(&g, 5, 5, true);
  game_set_cell(&g, 5, 6, true);
  game_set_cell(&g, 5, 7, true);
  assert(g.population == 3);

  /* Step 1: turns vertical -> (4, 6), (5, 6), (6, 6) */
  game_step_forward(&g);
  assert(g.population == 3);
  assert(g.grid[4][6] > 0);
  assert(g.grid[5][6] > 0);
  assert(g.grid[6][6] > 0);
  assert(g.grid[5][5] == 0);
  assert(g.grid[5][7] == 0);

  /* Step 2: turns back to horizontal */
  game_step_forward(&g);
  assert(g.population == 3);
  assert(g.grid[5][5] > 0);
  assert(g.grid[5][6] > 0);
  assert(g.grid[5][7] > 0);
  assert(g.grid[4][6] == 0);
  assert(g.grid[6][6] == 0);

  printf("[PASS] test_blinker_oscillator\n");
}

static void test_block_still_life(void) {
  GameState g;
  game_init(&g);
  game_clear(&g);

  /* 2x2 still life block at (2, 2) */
  game_set_cell(&g, 2, 2, true);
  game_set_cell(&g, 2, 3, true);
  game_set_cell(&g, 3, 2, true);
  game_set_cell(&g, 3, 3, true);
  assert(g.population == 4);

  for (int step = 0; step < 5; step++) {
    game_step_forward(&g);
    assert(g.population == 4);
    assert(g.grid[2][2] > 0 && g.grid[2][3] > 0);
    assert(g.grid[3][2] > 0 && g.grid[3][3] > 0);
  }

  printf("[PASS] test_block_still_life\n");
}

static void test_glider_translation(void) {
  GameState g;
  game_init(&g);
  game_clear(&g);

  /* Glider at top-left:
     .O.
     ..O
     OOO
  */
  game_stamp_preset(&g, 2, 2, 0); /* Preset 0 = Glider */
  assert(g.population == 5);

  /* A glider takes exactly 4 steps to move diagonally down-right by (+1, +1) */
  for (int step = 0; step < 4; step++) {
    game_step_forward(&g);
    assert(g.population == 5);
  }

  /* The glider shape is preserved at offset (3, 3):
     (3, 3) dead, (3, 4) alive, (3, 5) dead
     (4, 5) alive
     (5, 3), (5, 4), (5, 5) alive
  */
  assert(g.grid[3][4] > 0);
  assert(g.grid[4][5] > 0);
  assert(g.grid[5][3] > 0 && g.grid[5][4] > 0 && g.grid[5][5] > 0);

  printf("[PASS] test_glider_translation\n");
}

static void test_toroidal_wrapping(void) {
  GameState g;
  game_init(&g);
  game_clear(&g);

  g.wrap_mode = WRAP_TOROIDAL;
  /* Top-left corner (0, 0) and bottom-right corner (GRID_ROWS-1, GRID_COLS-1) */
  game_set_cell(&g, 0, 0, true);
  assert(game_count_neighbors(&g, GRID_ROWS - 1, GRID_COLS - 1) == 1);
  assert(game_count_neighbors(&g, 0, GRID_COLS - 1) == 1);
  assert(game_count_neighbors(&g, GRID_ROWS - 1, 0) == 1);

  /* Switch to bounded: wrapping should no longer count */
  g.wrap_mode = WRAP_BOUNDED;
  assert(game_count_neighbors(&g, GRID_ROWS - 1, GRID_COLS - 1) == 0);
  assert(game_count_neighbors(&g, 0, GRID_COLS - 1) == 0);

  printf("[PASS] test_toroidal_wrapping\n");
}

static void test_undo_step_back(void) {
  GameState g;
  game_init(&g);
  game_clear(&g);

  game_stamp_preset(&g, 5, 5, 0); /* Glider */
  assert(g.population == 5);
  assert(g.generation == 0);

  /* Step forward 4 times */
  for (int i = 0; i < 4; i++) {
    game_step_forward(&g);
  }
  assert(g.generation == 4);

  /* Step backward 2 times */
  assert(game_step_backward(&g));
  assert(g.generation == 3);
  assert(game_step_backward(&g));
  assert(g.generation == 2);

  printf("[PASS] test_undo_step_back\n");
}

static void test_preset_stamping(void) {
  GameState g;
  game_init(&g);

  for (int p = 0; p < MAX_PRESETS; p++) {
    game_clear(&g);
    game_stamp_preset(&g, 2, 2, p);
    assert(g.population > 0);
  }

  printf("[PASS] test_preset_stamping\n");
}

static void test_render_and_mouse_hit(void) {
  GameState g;
  game_init(&g);

  int r = -1, c = -1;
  /* Top-left cell (0, 0) is at mouse_x = 4..5, mouse_y = 4 */
  assert(render_get_cell_at_pos(4, 4, &r, &c));
  assert(r == 0 && c == 0);
  assert(render_get_cell_at_pos(5, 4, &r, &c));
  assert(r == 0 && c == 0);

  /* Bottom-right cell (17, 29) is at mouse_x = 62..63, mouse_y = 21 */
  assert(render_get_cell_at_pos(62, 21, &r, &c));
  assert(r == 17 && c == 29);

  /* Outside arena */
  assert(!render_get_cell_at_pos(1, 1, &r, &c));
  assert(!render_get_cell_at_pos(75, 10, &r, &c));

  /* Canary check for render_game */
  volatile int canary = 0x5A5A5A5A;
  render_game(&g);
  assert(canary == 0x5A5A5A5A);

  printf("[PASS] test_render_and_mouse_hit\n");
}

int main(void) {
  printf("Running Conway's Game of Life CLI unit tests...\n\n");

  test_blinker_oscillator();
  test_block_still_life();
  test_glider_translation();
  test_toroidal_wrapping();
  test_undo_step_back();
  test_preset_stamping();
  test_render_and_mouse_hit();

  printf("\nAll Conway's Game of Life CLI unit tests PASSED successfully!\n");
  return 0;
}
