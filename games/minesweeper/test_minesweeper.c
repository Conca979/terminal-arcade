#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "game.h"

static void test_initialization(void) {
  GameState game;
  game_init(&game);

  assert(game.difficulty == DIFF_BEGINNER);
  assert(game.rows == 9);
  assert(game.cols == 9);
  assert(game.total_mines == 10);
  assert(game.flags_placed == 0);
  assert(game.revealed_count == 0);
  assert(game.state == STATE_READY);
  printf("[PASS] test_initialization\n");
}

static void test_first_click_safety(void) {
  GameState game;
  game_init(&game);

  // Reveal center cell on first click
  game_reveal(&game, 4, 4, 1000);

  assert(game.state == STATE_PLAYING);
  assert(!game.grid[4][4].is_mine);
  assert(game.grid[4][4].is_revealed);
  assert(game.revealed_count > 0);
  printf("[PASS] test_first_click_safety\n");
}

static void test_flagging(void) {
  GameState game;
  game_init(&game);

  assert(!game.grid[0][0].is_flagged);
  game_toggle_flag(&game, 0, 0);
  assert(game.grid[0][0].is_flagged);
  assert(game.flags_placed == 1);

  // Unflag
  game_toggle_flag(&game, 0, 0);
  assert(!game.grid[0][0].is_flagged);
  assert(game.flags_placed == 0);
  printf("[PASS] test_flagging\n");
}

static void test_cursor_movement(void) {
  GameState game;
  game_init(&game);

  // Initial cursor at (4, 4)
  assert(game.cursor_r == 4 && game.cursor_c == 4);

  game_move_cursor(&game, -1, 0); // Up
  assert(game.cursor_r == 3);

  game_move_cursor(&game, 0, 1); // Right
  assert(game.cursor_c == 5);

  // Boundary clamp
  for (int i = 0; i < 20; i++) {
    game_move_cursor(&game, -1, -1);
  }
  assert(game.cursor_r == 0 && game.cursor_c == 0);
  printf("[PASS] test_cursor_movement\n");
}

static void test_difficulty_switch(void) {
  GameState game;
  game_init(&game);

  game_set_difficulty(&game, DIFF_EXPERT);
  assert(game.rows == 16);
  assert(game.cols == 30);
  assert(game.total_mines == 99);
  assert(game.state == STATE_READY);
  printf("[PASS] test_difficulty_switch\n");
}

int main(void) {
  srand(42);
  printf("Starting Minesweeper Logic Tests...\n");
  test_initialization();
  test_first_click_safety();
  test_flagging();
  test_cursor_movement();
  test_difficulty_switch();
  printf("All Minesweeper tests PASSED successfully!\n");
  return 0;
}
