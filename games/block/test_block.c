#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "game.h"

static void test_initialization(void) {
  GameState game;
  game_delete_save();
  game_reset(&game);

  assert(game.score == 0);
  assert(game.lines_cleared == 0);
  assert(game.streak == 0);
  assert(game.blocks_placed == 0);
  assert(!game.game_over);
  assert(!game.exit_requested);
  assert(game.cursor_r == 4 && game.cursor_c == 4);

  // Check 10x10 grid is completely empty
  for (int r = 0; r < GRID_SIZE; r++) {
    for (int c = 0; c < GRID_SIZE; c++) {
      assert(game.grid[r][c] == 0);
    }
  }

  // Check 3 tray pieces are generated and available
  for (int i = 0; i < MAX_PIECES_TRAY; i++) {
    assert(game.tray[i].available);
    assert(game.tray[i].shape.w >= 1 && game.tray[i].shape.w <= MAX_SHAPE_DIM);
    assert(game.tray[i].shape.h >= 1 && game.tray[i].shape.h <= MAX_SHAPE_DIM);
    assert(game.tray[i].shape.block_count >= 1 && game.tray[i].shape.block_count <= 9);
    assert(game.tray[i].shape.color_id >= 1 && game.tray[i].shape.color_id <= 5);
  }

  printf("[PASS] test_initialization (10x10 empty grid, 3 valid tray pieces)\n");
}

static void test_placement_and_bounds(void) {
  GameState game;
  game_reset(&game);

  // Manually configure tray slot 0 to a 2x2 square
  game.tray[0].available = true;
  game.tray[0].shape.w = 2;
  game.tray[0].shape.h = 2;
  game.tray[0].shape.block_count = 4;
  game.tray[0].shape.color_id = 2;
  memset(game.tray[0].shape.mask, 0, sizeof(game.tray[0].shape.mask));
  game.tray[0].shape.mask[0][0] = 1;
  game.tray[0].shape.mask[0][1] = 1;
  game.tray[0].shape.mask[1][0] = 1;
  game.tray[0].shape.mask[1][1] = 1;

  // Valid placement at (0, 0)
  assert(game_can_place(&game, 0, 0, 0));

  // Invalid placement out of bounds (bottom/right edge)
  assert(!game_can_place(&game, 0, 9, 9));  // 2x2 extends to row 10, col 10
  assert(!game_can_place(&game, 0, -1, 0));
  assert(!game_can_place(&game, 0, 0, -1));
  assert(!game_can_place(&game, 0, 9, 0));  // row 10 out of bounds
  assert(!game_can_place(&game, 0, 0, 9));  // col 10 out of bounds

  // Place at (2, 3)
  bool placed = game_place_piece(&game, 0, 2, 3);
  assert(placed);
  assert(!game.tray[0].available);
  assert(game.grid[2][3] == 2);
  assert(game.grid[2][4] == 2);
  assert(game.grid[3][3] == 2);
  assert(game.grid[3][4] == 2);
  assert(game.score == 40); // 4 blocks * 10
  assert(game.blocks_placed == 4);

  // Attempting to place another piece colliding at (2, 3) must fail
  game.tray[1].available = true;
  game.tray[1].shape.w = 1;
  game.tray[1].shape.h = 1;
  game.tray[1].shape.block_count = 1;
  game.tray[1].shape.mask[0][0] = 1;
  assert(!game_can_place(&game, 1, 2, 3));
  assert(!game_can_place(&game, 1, 3, 4));
  assert(game_can_place(&game, 1, 4, 4));

  printf("[PASS] test_placement_and_bounds (boundary checks, collisions, score)\n");
}

static void test_line_clear_horizontal(void) {
  GameState game;
  game_reset(&game);

  // Pre-fill row 4 from col 0 to 8
  for (int c = 0; c < 9; c++) {
    game.grid[4][c] = 1;
  }
  game.grid[4][9] = 0; // Empty slot

  // Configure tray 0 with a 1x1 dot piece
  game.tray[0].available = true;
  game.tray[0].shape.w = 1;
  game.tray[0].shape.h = 1;
  game.tray[0].shape.block_count = 1;
  game.tray[0].shape.color_id = 3;
  memset(game.tray[0].shape.mask, 0, sizeof(game.tray[0].shape.mask));
  game.tray[0].shape.mask[0][0] = 1;

  // Place dot at (4, 9) completing row 4
  bool placed = game_place_piece(&game, 0, 4, 9);
  assert(placed);

  // Entire row 4 should now be cleared back to 0
  for (int c = 0; c < GRID_SIZE; c++) {
    assert(game.grid[4][c] == 0);
  }

  assert(game.lines_cleared == 1);
  assert(game.streak == 1);
  // Score: 1 block (10 pts) + line clear bonus (100 pts) + streak bonus (50 pts) = 160 pts
  assert(game.score == 160);
  assert(strstr(game.combo_banner, "LINE CLEAR") != NULL);

  printf("[PASS] test_line_clear_horizontal (row 4 cleared, score = %d)\n", game.score);
}

static void test_line_clear_simultaneous_cross(void) {
  GameState game;
  game_reset(&game);

  // Fill row 5 except col 5
  for (int c = 0; c < GRID_SIZE; c++) {
    if (c != 5) game.grid[5][c] = 1;
  }
  // Fill col 5 except row 5
  for (int r = 0; r < GRID_SIZE; r++) {
    if (r != 5) game.grid[r][5] = 2;
  }
  assert(game.grid[5][5] == 0);

  // Prepare 1x1 dot
  game.tray[0].available = true;
  game.tray[0].shape.w = 1;
  game.tray[0].shape.h = 1;
  game.tray[0].shape.block_count = 1;
  game.tray[0].shape.color_id = 4;
  memset(game.tray[0].shape.mask, 0, sizeof(game.tray[0].shape.mask));
  game.tray[0].shape.mask[0][0] = 1;

  // Place dot at intersection (5, 5)
  bool placed = game_place_piece(&game, 0, 5, 5);
  assert(placed);

  // Both row 5 and col 5 must be cleared
  for (int c = 0; c < GRID_SIZE; c++) {
    assert(game.grid[5][c] == 0);
  }
  for (int r = 0; r < GRID_SIZE; r++) {
    assert(game.grid[r][5] == 0);
  }

  assert(game.lines_cleared == 2);
  assert(game.streak == 1);
  // Score: 10 + 300 (double clear) + 50 (streak) = 360
  assert(game.score == 360);
  assert(strstr(game.combo_banner, "DOUBLE CLEAR") != NULL);

  printf("[PASS] test_line_clear_simultaneous_cross (horizontal + vertical simultaneous clear)\n");
}

static void test_tray_refill(void) {
  GameState game;
  game_reset(&game);

  // Setup all 3 pieces as 1x1 dots
  for (int i = 0; i < MAX_PIECES_TRAY; i++) {
    game.tray[i].available = true;
    game.tray[i].shape.w = 1;
    game.tray[i].shape.h = 1;
    game.tray[i].shape.block_count = 1;
    memset(game.tray[i].shape.mask, 0, sizeof(game.tray[i].shape.mask));
    game.tray[i].shape.mask[0][0] = 1;
  }

  // Place first two pieces
  assert(game_place_piece(&game, 0, 0, 0));
  assert(!game.tray[0].available);
  assert(game.tray[1].available);
  assert(game.tray[2].available);

  assert(game_place_piece(&game, 1, 0, 1));
  assert(!game.tray[0].available);
  assert(!game.tray[1].available);
  assert(game.tray[2].available);

  // Place 3rd piece -> triggers refill of all 3 slots
  assert(game_place_piece(&game, 2, 0, 2));
  assert(game.tray[0].available);
  assert(game.tray[1].available);
  assert(game.tray[2].available);

  printf("[PASS] test_tray_refill (tray automatically replenished upon exhaustion)\n");
}

static void test_game_over_detection(void) {
  GameState game;
  game_reset(&game);

  // Fill the entire 10x10 grid with blocks
  for (int r = 0; r < GRID_SIZE; r++) {
    for (int c = 0; c < GRID_SIZE; c++) {
      game.grid[r][c] = 1;
    }
  }

  // Open only a single cell at (0, 0)
  game.grid[0][0] = 0;

  // Set tray piece to a 2x2 square (cannot fit into 1x1)
  game.tray[0].available = true;
  game.tray[0].shape.w = 2;
  game.tray[0].shape.h = 2;
  game.tray[0].shape.block_count = 4;
  memset(game.tray[0].shape.mask, 0, sizeof(game.tray[0].shape.mask));
  game.tray[0].shape.mask[0][0] = 1;
  game.tray[0].shape.mask[0][1] = 1;
  game.tray[0].shape.mask[1][0] = 1;
  game.tray[0].shape.mask[1][1] = 1;

  game.tray[1].available = false;
  game.tray[2].available = false;

  // No valid move exists
  assert(!game_has_any_valid_move(&game));

  // Now change piece to a 1x1 dot
  game.tray[0].shape.w = 1;
  game.tray[0].shape.h = 1;
  game.tray[0].shape.block_count = 1;
  memset(game.tray[0].shape.mask, 0, sizeof(game.tray[0].shape.mask));
  game.tray[0].shape.mask[0][0] = 1;

  // Valid move should now exist at (0, 0)
  assert(game_has_any_valid_move(&game));
  assert(game_can_place(&game, 0, 0, 0));

  printf("[PASS] test_game_over_detection (detected when no tray shapes fit)\n");
}

static void test_save_and_resume(void) {
  GameState game;
  game_delete_save();
  game_reset(&game);

  game.score = 5480;
  game.lines_cleared = 18;
  game.streak = 3;
  game.blocks_placed = 42;
  game.grid[1][2] = 3;
  game.grid[4][7] = 5;

  game.tray[0].available = false;
  game.tray[1].available = true;
  game.tray[1].shape.w = 3;
  game.tray[1].shape.h = 1;
  game.tray[1].shape.block_count = 3;
  game.tray[1].shape.color_id = 4;

  game_save(&game);

  GameState loaded;
  memset(&loaded, 0, sizeof(GameState));
  bool success = game_load(&loaded);

  assert(success);
  assert(loaded.score == 5480);
  assert(loaded.lines_cleared == 18);
  assert(loaded.streak == 3);
  assert(loaded.blocks_placed == 42);
  assert(loaded.grid[1][2] == 3);
  assert(loaded.grid[4][7] == 5);
  assert(!loaded.tray[0].available);
  assert(loaded.tray[1].available);
  assert(loaded.tray[1].shape.w == 3);
  assert(loaded.resumed_game);

  game_delete_save();
  printf("[PASS] test_save_and_resume (persisted state, score, grid, and tray)\n");
}

int main(void) {
  printf("Starting Wood Block Puzzle Logic & Rules Tests...\n");
  test_initialization();
  test_placement_and_bounds();
  test_line_clear_horizontal();
  test_line_clear_simultaneous_cross();
  test_tray_refill();
  test_game_over_detection();
  test_save_and_resume();
  printf("All Wood Block Puzzle tests PASSED successfully!\n");
  return 0;
}
