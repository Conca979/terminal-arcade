#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "game.h"

static void test_initialization(void) {
  GameState game;
  game_delete_save();
  game_init(&game);

  assert(game.score == 0);
  assert(game.move_count == 0);
  assert(game.state == STATE_PLAYING);
  assert(!game.won_acknowledged);

  // Exactly two tiles spawned
  int tile_count = 0;
  for (int r = 0; r < GRID_SIZE; r++) {
    for (int c = 0; c < GRID_SIZE; c++) {
      if (game.board[r][c] != 0) {
        assert(game.board[r][c] == 2 || game.board[r][c] == 4);
        tile_count++;
      }
    }
  }
  assert(tile_count == 2);
  printf("[PASS] test_initialization\n");
}

static void test_slide_and_merge_left(void) {
  GameState game;
  memset(&game, 0, sizeof(game));

  // Row 0: [2, 2, 2, 2] -> should merge to [4, 4, 0, 0]
  game.board[0][0] = 2;
  game.board[0][1] = 2;
  game.board[0][2] = 2;
  game.board[0][3] = 2;

  // Row 1: [2, 0, 2, 4] -> should merge to [4, 4, 0, 0]
  game.board[1][0] = 2;
  game.board[1][1] = 0;
  game.board[1][2] = 2;
  game.board[1][3] = 4;

  bool moved = game_slide(&game, DIR_LEFT);
  assert(moved);

  // Check row 0
  assert(game.board[0][0] == 4);
  assert(game.board[0][1] == 4);

  // Check row 1
  assert(game.board[1][0] == 4);
  assert(game.board[1][1] == 4);

  // Score gain should be (4 + 4) from row 0 + (4) from row 1 = 12
  assert(game.score == 12);
  printf("[PASS] test_slide_and_merge_left\n");
}

static void test_undo(void) {
  GameState game;
  memset(&game, 0, sizeof(game));
  game.board[0][0] = 2;
  game.board[0][1] = 2;

  int initial_score = game.score;

  bool moved = game_slide(&game, DIR_LEFT);
  assert(moved);
  assert(game.score == initial_score + 4);
  assert(game.history_count == 1);

  bool undone = game_undo(&game);
  assert(undone);
  assert(game.board[0][0] == 2);
  assert(game.board[0][1] == 2);
  assert(game.score == initial_score);
  assert(game.history_count == 0);

  // Undoing again when history is empty should return false
  assert(!game_undo(&game));
  printf("[PASS] test_undo\n");
}

static void test_win_and_continue(void) {
  GameState game;
  memset(&game, 0, sizeof(game));
  game.board[0][0] = 1024;
  game.board[0][1] = 1024;

  bool moved = game_slide(&game, DIR_LEFT);
  assert(moved);
  assert(game.board[0][0] == 2048);
  assert(game.state == STATE_WON);

  // Player acknowledges and continues
  game.won_acknowledged = true;
  game.state = STATE_PLAYING;

  // Next move should not trigger win modal again
  game.board[1][0] = 2;
  game.board[1][1] = 2;
  game_slide(&game, DIR_LEFT);
  assert(game.state == STATE_PLAYING);
  printf("[PASS] test_win_and_continue\n");
}

static void test_game_over(void) {
  GameState game;
  memset(&game, 0, sizeof(game));

  // Fill board with checkerboard pattern (no adjacent equal values)
  int val = 2;
  for (int r = 0; r < GRID_SIZE; r++) {
    for (int c = 0; c < GRID_SIZE; c++) {
      game.board[r][c] = val;
      val *= 2;
    }
  }

  assert(!game_can_move(&game));
  bool moved = game_slide(&game, DIR_LEFT);
  assert(!moved);
  printf("[PASS] test_game_over\n");
}

static void test_auto_save_and_resume(void) {
  game_delete_save();
  GameState g1;
  game_init(&g1);

  g1.board[0][0] = 64;
  g1.board[0][1] = 128;
  g1.score = 500;
  g1.move_count = 15;
  game_save(&g1);

  GameState g2;
  game_init(&g2);
  assert(g2.score == 500);
  assert(g2.move_count == 15);
  assert(g2.board[0][0] == 64);
  assert(g2.board[0][1] == 128);

  game_delete_save();
  printf("[PASS] test_auto_save_and_resume\n");
}

int main(void) {
  srand(42);
  printf("Starting 2048 Logic Tests...\n");
  test_initialization();
  test_slide_and_merge_left();
  test_undo();
  test_win_and_continue();
  test_game_over();
  test_auto_save_and_resume();
  printf("All 2048 tests PASSED successfully!\n");
  return 0;
}

