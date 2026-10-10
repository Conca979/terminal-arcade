#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "game.h"

static void test_initialization(void) {
  GameState game;
  game_init(&game);

  assert(game.score == 0);
  assert(game.level == 1);
  assert(game.lines_cleared == 0);
  assert(game.state == STATE_PLAYING);
  assert(game.current.type >= TET_I && game.current.type <= TET_Z);
  assert(game.can_hold == true);
  assert(game.hold_piece == TET_NONE);
  printf("[PASS] test_initialization\n");
}

static void test_ghost_calculation(void) {
  GameState game;
  game_init(&game);

  int ghost_y = game_get_ghost_y(&game);
  assert(ghost_y >= game.current.y);
  assert(ghost_y < TOTAL_HEIGHT);
  printf("[PASS] test_ghost_calculation (ghost_y = %d)\n", ghost_y);
}

static void test_movement_and_collision(void) {
  GameState game;
  game_init(&game);

  // Move left until boundary
  for (int i = 0; i < 15; i++) {
    game_handle_action(&game, ACT_LEFT, 100);
  }
  int left_x = game.current.x;
  ActivePiece test_left = game.current;
  test_left.x--;
  assert(game_check_collision(&game, &test_left));

  // Move right until boundary
  for (int i = 0; i < 20; i++) {
    game_handle_action(&game, ACT_RIGHT, 200);
  }
  int right_x = game.current.x;
  assert(right_x > left_x);
  ActivePiece test_right = game.current;
  test_right.x++;
  assert(game_check_collision(&game, &test_right));

  printf("[PASS] test_movement_and_collision\n");
}

static void test_hard_drop(void) {
  GameState game;
  game_init(&game);

  TetrominoType first_type = game.current.type;
  game_handle_action(&game, ACT_HARD_DROP, 100);

  // After hard drop, score should be > 0 and piece should have locked / spawned new piece
  assert(game.score > 0);
  // Bottom row should now have blocks
  bool found_bottom = false;
  for (int x = 0; x < BOARD_WIDTH; x++) {
    if (game.board[TOTAL_HEIGHT - 1][x] == first_type) {
      found_bottom = true;
      break;
    }
  }
  assert(found_bottom);
  printf("[PASS] test_hard_drop (score = %d)\n", game.score);
}

static void test_line_clear_and_scoring(void) {
  GameState game;
  game_init(&game);

  // Fill bottom row except column 0
  for (int x = 1; x < BOARD_WIDTH; x++) {
    game.board[TOTAL_HEIGHT - 1][x] = TET_I;
  }
  // Fill column 0
  game.board[TOTAL_HEIGHT - 1][0] = TET_I;

  // Simulate locking a piece to trigger line clear detection
  game.current.type = TET_I;
  game.current.rotation = 0;
  game.current.x = 0;
  game.current.y = TOTAL_HEIGHT - 5;
  game_handle_action(&game, ACT_HARD_DROP, 100);

  // The bottom line should be detected for clear
  assert(game.state == STATE_LINE_CLEAR || game.lines_cleared > 0);

  // Advance time to complete line clear
  game_update(&game, 500);

  assert(game.lines_cleared >= 1);
  assert(game.score >= 100);
  printf("[PASS] test_line_clear_and_scoring (lines = %d, score = %d)\n", game.lines_cleared, game.score);
}

static void test_hold_mechanic(void) {
  GameState game;
  game_init(&game);

  TetrominoType initial_piece = game.current.type;
  game_handle_action(&game, ACT_HOLD, 100);

  assert(game.hold_piece == initial_piece);
  assert(!game.can_hold); // Can't hold twice in a row

  TetrominoType piece_after_hold = game.current.type;
  // Second hold should not do anything until piece drops
  game_handle_action(&game, ACT_HOLD, 110);
  assert(game.current.type == piece_after_hold);

  printf("[PASS] test_hold_mechanic\n");
}

int main(void) {
  srand(42);
  printf("Starting Tetris Logic Tests...\n");
  test_initialization();
  test_ghost_calculation();
  test_movement_and_collision();
  test_hard_drop();
  test_line_clear_and_scoring();
  test_hold_mechanic();
  printf("All tests PASSED successfully!\n");
  return 0;
}
