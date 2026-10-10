#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "game.h"

static void test_initialization_and_generation(void) {
  GameState game;
  game_init(&game, DIFF_EASY);

  assert(game.state == STATE_PLAYING);
  assert(game.difficulty == DIFF_EASY);
  assert(game.cursor_r == 4);
  assert(game.cursor_c == 4);
  assert(game.mistakes == 0);
  assert(game.moves_made == 0);

  // Validate complete solution matrix has digits 1..9 in all rows, cols, and boxes
  for (int r = 0; r < GRID_SIZE; r++) {
    int row_mask = 0;
    for (int c = 0; c < GRID_SIZE; c++) {
      uint8_t sol = game.grid[r][c].solution;
      assert(sol >= 1 && sol <= 9);
      row_mask |= (1 << sol);
    }
    assert(row_mask == 0x3FE); // Bits 1..9 set
  }

  for (int c = 0; c < GRID_SIZE; c++) {
    int col_mask = 0;
    for (int r = 0; r < GRID_SIZE; r++) {
      uint8_t sol = game.grid[r][c].solution;
      col_mask |= (1 << sol);
    }
    assert(col_mask == 0x3FE);
  }

  int given_count = 0;
  for (int r = 0; r < GRID_SIZE; r++) {
    for (int c = 0; c < GRID_SIZE; c++) {
      if (game.grid[r][c].is_given) {
        assert(game.grid[r][c].value == game.grid[r][c].solution);
        given_count++;
      }
    }
  }

  // Easy difficulty should have plenty of clues
  assert(given_count >= 30 && given_count <= 45);
  printf("[PASS] test_initialization_and_generation (clues = %d)\n", given_count);
}

static void test_cell_input_and_undo(void) {
  GameState game;
  game_init(&game, DIFF_EASY);

  // Find an empty cell
  int empty_r = -1, empty_c = -1;
  for (int r = 0; r < GRID_SIZE && empty_r == -1; r++) {
    for (int c = 0; c < GRID_SIZE; c++) {
      if (!game.grid[r][c].is_given) {
        empty_r = r;
        empty_c = c;
        break;
      }
    }
  }
  assert(empty_r != -1);

  // Test placing a number
  uint8_t test_val = game.grid[empty_r][empty_c].solution;
  bool ok = game_set_cell(&game, empty_r, empty_c, test_val);
  assert(ok);
  assert(game.grid[empty_r][empty_c].value == test_val);
  assert(game.moves_made == 1);
  assert(game.undo_count == 1);

  // Test undo
  bool undone = game_undo(&game);
  assert(undone);
  assert(game.grid[empty_r][empty_c].value == 0);
  assert(game.undo_count == 0);

  printf("[PASS] test_cell_input_and_undo\n");
}

static void test_conflict_detection(void) {
  GameState game;
  game_init(&game, DIFF_EASY);

  // Find two empty cells in the same row
  int r_target = -1;
  int c1 = -1, c2 = -1;
  for (int r = 0; r < GRID_SIZE && r_target == -1; r++) {
    int count = 0;
    int first_c = -1, second_c = -1;
    for (int c = 0; c < GRID_SIZE; c++) {
      if (!game.grid[r][c].is_given) {
        if (count == 0) first_c = c;
        else if (count == 1) second_c = c;
        count++;
      }
    }
    if (count >= 2) {
      r_target = r;
      c1 = first_c;
      c2 = second_c;
    }
  }
  assert(r_target != -1);

  // Place duplicate value '5' in both cells
  game_set_cell(&game, r_target, c1, 5);
  game_set_cell(&game, r_target, c2, 5);

  assert(game.grid[r_target][c1].is_error == true);
  assert(game.grid[r_target][c2].is_error == true);

  // Clear one cell
  game_clear_cell(&game, r_target, c2);
  assert(game.grid[r_target][c2].value == 0);
  assert(game.grid[r_target][c2].is_error == false);

  printf("[PASS] test_conflict_detection\n");
}

static void test_notes_mode(void) {
  GameState game;
  game_init(&game, DIFF_EASY);

  // Find empty cell
  int er = -1, ec = -1;
  for (int r = 0; r < GRID_SIZE && er == -1; r++) {
    for (int c = 0; c < GRID_SIZE; c++) {
      if (!game.grid[r][c].is_given) {
        er = r;
        ec = c;
        break;
      }
    }
  }

  // Toggle notes 3 and 7
  game_toggle_note(&game, er, ec, 3);
  assert((game.grid[er][ec].notes & (1 << 3)) != 0);
  assert((game.grid[er][ec].notes & (1 << 7)) == 0);

  game_toggle_note(&game, er, ec, 7);
  assert((game.grid[er][ec].notes & (1 << 3)) != 0);
  assert((game.grid[er][ec].notes & (1 << 7)) != 0);

  // Toggle note 3 off
  game_toggle_note(&game, er, ec, 3);
  assert((game.grid[er][ec].notes & (1 << 3)) == 0);
  assert((game.grid[er][ec].notes & (1 << 7)) != 0);

  printf("[PASS] test_notes_mode\n");
}

static void test_hint_and_victory(void) {
  GameState game;
  game_init(&game, DIFF_EASY);

  // Fill all empty cells with their known solutions except the last one
  int last_r = -1, last_c = -1;
  for (int r = 0; r < GRID_SIZE; r++) {
    for (int c = 0; c < GRID_SIZE; c++) {
      if (!game.grid[r][c].is_given) {
        if (last_r == -1) {
          last_r = r;
          last_c = c;
        } else {
          game.grid[r][c].value = game.grid[r][c].solution;
        }
      }
    }
  }
  assert(last_r != -1);
  game_check_conflicts(&game);
  assert(game.state == STATE_PLAYING);

  // Set cursor to last cell and use hint
  game.cursor_r = (uint8_t)last_r;
  game.cursor_c = (uint8_t)last_c;
  bool hint_ok = game_hint(&game);
  assert(hint_ok);
  assert(game.hints_used == 1);
  assert(game.grid[last_r][last_c].value == game.grid[last_r][last_c].solution);
  assert(game.state == STATE_VICTORY);

  printf("[PASS] test_hint_and_victory\n");
}

int main(void) {
  printf("Starting Sudoku Logic Tests...\n");
  test_initialization_and_generation();
  test_cell_input_and_undo();
  test_conflict_detection();
  test_notes_mode();
  test_hint_and_victory();
  printf("All Sudoku tests PASSED successfully!\n");
  return 0;
}
