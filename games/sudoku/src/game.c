#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

static const uint8_t BASE_GRID[GRID_SIZE][GRID_SIZE] = {
  {1, 2, 3, 4, 5, 6, 7, 8, 9},
  {4, 5, 6, 7, 8, 9, 1, 2, 3},
  {7, 8, 9, 1, 2, 3, 4, 5, 6},
  {2, 3, 1, 5, 6, 4, 8, 9, 7},
  {5, 6, 4, 8, 9, 7, 2, 3, 1},
  {8, 9, 7, 2, 3, 1, 5, 6, 4},
  {3, 1, 2, 6, 4, 5, 9, 7, 8},
  {6, 4, 5, 9, 7, 8, 3, 1, 2},
  {9, 7, 8, 3, 1, 2, 6, 4, 5}
};

static bool is_valid_in_grid(const uint8_t grid[GRID_SIZE][GRID_SIZE], int r, int c, uint8_t val) {
  for (int i = 0; i < GRID_SIZE; i++) {
    if (i != c && grid[r][i] == val) return false;
    if (i != r && grid[i][c] == val) return false;
  }

  int start_r = (r / BOX_SIZE) * BOX_SIZE;
  int start_c = (c / BOX_SIZE) * BOX_SIZE;
  for (int br = 0; br < BOX_SIZE; br++) {
    for (int bc = 0; bc < BOX_SIZE; bc++) {
      int tr = start_r + br;
      int tc = start_c + bc;
      if ((tr != r || tc != c) && grid[tr][tc] == val) {
        return false;
      }
    }
  }

  return true;
}

static int count_solutions(uint8_t grid[GRID_SIZE][GRID_SIZE], int count, int limit) {
  if (count >= limit) return count;

  // Find empty cell with minimum remaining values (MRV)
  int best_r = -1;
  int best_c = -1;
  int min_candidates = 10;

  for (int r = 0; r < GRID_SIZE; r++) {
    for (int c = 0; c < GRID_SIZE; c++) {
      if (grid[r][c] == 0) {
        int candidate_count = 0;
        for (uint8_t v = 1; v <= 9; v++) {
          if (is_valid_in_grid(grid, r, c, v)) {
            candidate_count++;
          }
        }
        if (candidate_count < min_candidates) {
          min_candidates = candidate_count;
          best_r = r;
          best_c = c;
          if (min_candidates <= 1) break;
        }
      }
    }
    if (min_candidates <= 1) break;
  }

  if (best_r == -1) {
    return count + 1; // All cells filled
  }

  if (min_candidates == 0) {
    return count; // Unsolvable branch
  }

  for (uint8_t v = 1; v <= 9; v++) {
    if (is_valid_in_grid(grid, best_r, best_c, v)) {
      grid[best_r][best_c] = v;
      count = count_solutions(grid, count, limit);
      grid[best_r][best_c] = 0;
      if (count >= limit) break;
    }
  }

  return count;
}

static void update_digit_counts(GameState *game) {
  for (int i = 0; i <= 9; i++) {
    game->digit_counts[i] = 0;
  }

  for (int r = 0; r < GRID_SIZE; r++) {
    for (int c = 0; c < GRID_SIZE; c++) {
      uint8_t v = game->grid[r][c].value;
      if (v >= 1 && v <= 9) {
        game->digit_counts[v]++;
      }
    }
  }

  for (int i = 1; i <= 9; i++) {
    game->remaining_counts[i] = 9 - game->digit_counts[i];
    if (game->remaining_counts[i] < 0) {
      game->remaining_counts[i] = 0;
    }
  }
}

void game_check_conflicts(GameState *game) {
  for (int r = 0; r < GRID_SIZE; r++) {
    for (int c = 0; c < GRID_SIZE; c++) {
      game->grid[r][c].is_error = false;
    }
  }

  for (int r = 0; r < GRID_SIZE; r++) {
    for (int c = 0; c < GRID_SIZE; c++) {
      uint8_t val = game->grid[r][c].value;
      if (val == 0) continue;

      // Row check
      for (int i = 0; i < GRID_SIZE; i++) {
        if (i != c && game->grid[r][i].value == val) {
          game->grid[r][c].is_error = true;
          game->grid[r][i].is_error = true;
        }
      }

      // Column check
      for (int i = 0; i < GRID_SIZE; i++) {
        if (i != r && game->grid[i][c].value == val) {
          game->grid[r][c].is_error = true;
          game->grid[i][c].is_error = true;
        }
      }

      // 3x3 Block check
      int start_r = (r / BOX_SIZE) * BOX_SIZE;
      int start_c = (c / BOX_SIZE) * BOX_SIZE;
      for (int br = 0; br < BOX_SIZE; br++) {
        for (int bc = 0; bc < BOX_SIZE; bc++) {
          int tr = start_r + br;
          int tc = start_c + bc;
          if ((tr != r || tc != c) && game->grid[tr][tc].value == val) {
            game->grid[r][c].is_error = true;
            game->grid[tr][tc].is_error = true;
          }
        }
      }
    }
  }

  update_digit_counts(game);
}

void game_generate_puzzle(GameState *game, DifficultyLevel diff) {
  uint8_t solution[GRID_SIZE][GRID_SIZE];
  memcpy(solution, BASE_GRID, sizeof(solution));

  // 1. Permute numbers 1..9
  uint8_t num_map[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  for (int i = 1; i <= 9; i++) {
    int swap_idx = 1 + (rand() % (10 - i));
    uint8_t temp = num_map[i];
    num_map[i] = num_map[swap_idx];
    num_map[swap_idx] = temp;
  }

  for (int r = 0; r < GRID_SIZE; r++) {
    for (int c = 0; c < GRID_SIZE; c++) {
      solution[r][c] = num_map[solution[r][c]];
    }
  }

  // 2. Permute rows within 3x3 blocks
  for (int band = 0; band < 3; band++) {
    for (int i = 0; i < 3; i++) {
      int r1 = band * 3 + i;
      int r2 = band * 3 + (rand() % 3);
      if (r1 != r2) {
        for (int c = 0; c < GRID_SIZE; c++) {
          uint8_t tmp = solution[r1][c];
          solution[r1][c] = solution[r2][c];
          solution[r2][c] = tmp;
        }
      }
    }
  }

  // 3. Permute cols within 3x3 blocks
  for (int stack = 0; stack < 3; stack++) {
    for (int i = 0; i < 3; i++) {
      int c1 = stack * 3 + i;
      int c2 = stack * 3 + (rand() % 3);
      if (c1 != c2) {
        for (int r = 0; r < GRID_SIZE; r++) {
          uint8_t tmp = solution[r][c1];
          solution[r][c1] = solution[r][c2];
          solution[r][c2] = tmp;
        }
      }
    }
  }

  // 4. Permute entire 3x3 bands
  for (int b = 0; b < 3; b++) {
    int target_b = rand() % 3;
    if (b != target_b) {
      for (int i = 0; i < 3; i++) {
        for (int c = 0; c < GRID_SIZE; c++) {
          uint8_t tmp = solution[b * 3 + i][c];
          solution[b * 3 + i][c] = solution[target_b * 3 + i][c];
          solution[target_b * 3 + i][c] = tmp;
        }
      }
    }
  }

  // 5. Randomly transpose
  if (rand() % 2 == 0) {
    for (int r = 0; r < GRID_SIZE; r++) {
      for (int c = r + 1; c < GRID_SIZE; c++) {
        uint8_t tmp = solution[r][c];
        solution[r][c] = solution[c][r];
        solution[c][r] = tmp;
      }
    }
  }

  // Setup game grid with complete solution
  for (int r = 0; r < GRID_SIZE; r++) {
    for (int c = 0; c < GRID_SIZE; c++) {
      game->grid[r][c].solution = solution[r][c];
      game->grid[r][c].value = solution[r][c];
      game->grid[r][c].is_given = true;
      game->grid[r][c].is_error = false;
      game->grid[r][c].notes = 0;
    }
  }

  // Number of cells to remove based on difficulty
  int target_removals = 40; // Easy
  switch (diff) {
    case DIFF_EASY:   target_removals = 42; break; // ~39 clues
    case DIFF_MEDIUM: target_removals = 50; break; // ~31 clues
    case DIFF_HARD:   target_removals = 55; break; // ~26 clues
    case DIFF_EXPERT: target_removals = 58; break; // ~23 clues
    default: break;
  }

  // Shuffle cell coordinates for removal
  int coords[TOTAL_CELLS];
  for (int i = 0; i < TOTAL_CELLS; i++) {
    coords[i] = i;
  }
  for (int i = 0; i < TOTAL_CELLS; i++) {
    int swap_idx = i + (rand() % (TOTAL_CELLS - i));
    int tmp = coords[i];
    coords[i] = coords[swap_idx];
    coords[swap_idx] = tmp;
  }

  uint8_t test_grid[GRID_SIZE][GRID_SIZE];
  for (int r = 0; r < GRID_SIZE; r++) {
    for (int c = 0; c < GRID_SIZE; c++) {
      test_grid[r][c] = solution[r][c];
    }
  }

  int removed = 0;
  for (int i = 0; i < TOTAL_CELLS && removed < target_removals; i++) {
    int r = coords[i] / GRID_SIZE;
    int c = coords[i] % GRID_SIZE;

    uint8_t prev = test_grid[r][c];
    test_grid[r][c] = 0;

    // Check if unique solution exists
    uint8_t check_copy[GRID_SIZE][GRID_SIZE];
    memcpy(check_copy, test_grid, sizeof(check_copy));
    int solutions = count_solutions(check_copy, 0, 2);

    if (solutions == 1) {
      game->grid[r][c].value = 0;
      game->grid[r][c].is_given = false;
      removed++;
    } else {
      // Restore if removal breaks uniqueness
      test_grid[r][c] = prev;
    }
  }

  game->undo_count = 0;
  game_check_conflicts(game);
}

int game_get_elapsed_seconds(const GameState *game, unsigned long long current_time) {
  unsigned long long total_ms = game->saved_elapsed_ms;
  if (game->start_time_ms != 0) {
    if (game->state == STATE_PAUSED) {
      total_ms += (game->pause_start_ms - game->start_time_ms - game->elapsed_paused_ms);
    } else {
      total_ms += (current_time - game->start_time_ms - game->elapsed_paused_ms);
    }
  }
  return (int)(total_ms / 1000ULL);
}

static const char *get_save_path(void) {
  FILE *f = fopen("games/sudoku/Makefile", "r");
  if (f) {
    fclose(f);
    return "games/sudoku/savegame.dat";
  }
  return "savegame.dat";
}

#define SAVE_MAGIC_SUDOKU 0x5355444F // "SUDO"
#define SAVE_VERSION_SUDOKU 1

void game_delete_save(void) {
  remove(get_save_path());
}

void game_save(const GameState *game, unsigned long long current_time) {
  if (game->state == STATE_VICTORY) return;

  FILE *f = fopen(get_save_path(), "wb");
  if (!f) return;

  uint32_t magic = SAVE_MAGIC_SUDOKU;
  uint32_t ver = SAVE_VERSION_SUDOKU;
  fwrite(&magic, sizeof(uint32_t), 1, f);
  fwrite(&ver, sizeof(uint32_t), 1, f);

  fwrite(game->grid, sizeof(Cell), GRID_SIZE * GRID_SIZE, f);
  fwrite(&game->cursor_r, sizeof(uint8_t), 1, f);
  fwrite(&game->cursor_c, sizeof(uint8_t), 1, f);

  int diff = (int)game->difficulty;
  fwrite(&diff, sizeof(int), 1, f);
  int note_mode = game->note_mode ? 1 : 0;
  fwrite(&note_mode, sizeof(int), 1, f);
  int show_errors = game->show_errors ? 1 : 0;
  fwrite(&show_errors, sizeof(int), 1, f);
  fwrite(&game->mistakes, sizeof(int), 1, f);
  fwrite(&game->hints_used, sizeof(int), 1, f);
  fwrite(&game->moves_made, sizeof(int), 1, f);

  int elapsed_sec = game_get_elapsed_seconds(game, current_time);
  fwrite(&elapsed_sec, sizeof(int), 1, f);

  fwrite(&game->undo_count, sizeof(int), 1, f);
  if (game->undo_count > 0 && game->undo_count <= MAX_UNDO_HISTORY) {
    fwrite(game->undo_stack, sizeof(UndoStep), game->undo_count, f);
  }

  fclose(f);
}

bool game_load(GameState *game) {
  FILE *f = fopen(get_save_path(), "rb");
  if (!f) return false;

  uint32_t magic = 0;
  uint32_t ver = 0;
  if (fread(&magic, sizeof(uint32_t), 1, f) != 1 || magic != SAVE_MAGIC_SUDOKU) {
    fclose(f);
    return false;
  }
  if (fread(&ver, sizeof(uint32_t), 1, f) != 1 || ver != SAVE_VERSION_SUDOKU) {
    fclose(f);
    return false;
  }

  if (fread(game->grid, sizeof(Cell), GRID_SIZE * GRID_SIZE, f) != GRID_SIZE * GRID_SIZE ||
      fread(&game->cursor_r, sizeof(uint8_t), 1, f) != 1 ||
      fread(&game->cursor_c, sizeof(uint8_t), 1, f) != 1) {
    fclose(f);
    return false;
  }

  int diff = 0, note_mode = 0, show_errors = 0;
  if (fread(&diff, sizeof(int), 1, f) != 1 ||
      fread(&note_mode, sizeof(int), 1, f) != 1 ||
      fread(&show_errors, sizeof(int), 1, f) != 1 ||
      fread(&game->mistakes, sizeof(int), 1, f) != 1 ||
      fread(&game->hints_used, sizeof(int), 1, f) != 1 ||
      fread(&game->moves_made, sizeof(int), 1, f) != 1) {
    fclose(f);
    return false;
  }

  int elapsed_sec = 0;
  if (fread(&elapsed_sec, sizeof(int), 1, f) != 1) {
    fclose(f);
    return false;
  }

  if (fread(&game->undo_count, sizeof(int), 1, f) != 1) {
    fclose(f);
    return false;
  }

  if (game->undo_count < 0 || game->undo_count > MAX_UNDO_HISTORY) {
    game->undo_count = 0;
  } else if (game->undo_count > 0) {
    if ((int)fread(game->undo_stack, sizeof(UndoStep), game->undo_count, f) != game->undo_count) {
      game->undo_count = 0;
    }
  }

  fclose(f);

  if (diff < 0 || diff >= DIFF_COUNT) diff = DIFF_MEDIUM;
  game->difficulty = (DifficultyLevel)diff;
  game->note_mode = (note_mode != 0);
  game->show_errors = (show_errors != 0);

  if (game->cursor_r >= GRID_SIZE) game->cursor_r = 4;
  if (game->cursor_c >= GRID_SIZE) game->cursor_c = 4;

  game->state = STATE_PLAYING;
  game->saved_elapsed_ms = (unsigned long long)elapsed_sec * 1000ULL;
  game->start_time_ms = 0;
  game->elapsed_paused_ms = 0;
  game->pause_start_ms = 0;

  game_check_conflicts(game);

  if (game_check_victory(game)) {
    game_delete_save();
    return false;
  }

  return true;
}

void game_init(GameState *game, DifficultyLevel diff) {
  memset(game, 0, sizeof(GameState));
  if (game_load(game)) {
    return;
  }

  game->cursor_r = 4;
  game->cursor_c = 4;
  game->difficulty = diff;
  game->state = STATE_PLAYING;
  game->note_mode = false;
  game->show_errors = true;
  game->mistakes = 0;
  game->hints_used = 0;
  game->moves_made = 0;
  game->start_time_ms = 0;
  game->elapsed_paused_ms = 0;
  game->pause_start_ms = 0;
  game->saved_elapsed_ms = 0;
  game->undo_count = 0;

  game_generate_puzzle(game, diff);
  game_save(game, 0);
}

void game_reset_board(GameState *game) {
  for (int r = 0; r < GRID_SIZE; r++) {
    for (int c = 0; c < GRID_SIZE; c++) {
      if (!game->grid[r][c].is_given) {
        game->grid[r][c].value = 0;
        game->grid[r][c].notes = 0;
      }
    }
  }
  game->undo_count = 0;
  game->mistakes = 0;
  game->moves_made = 0;
  game->hints_used = 0;
  game_check_conflicts(game);
}

bool game_set_cell(GameState *game, int r, int c, int val) {
  if (r < 0 || r >= GRID_SIZE || c < 0 || c >= GRID_SIZE) return false;
  Cell *cell = &game->grid[r][c];
  if (cell->is_given) return false;

  // Toggle off if pressing the same value
  uint8_t new_val = (cell->value == val) ? 0 : (uint8_t)val;

  if (game->undo_count < MAX_UNDO_HISTORY) {
    UndoStep *step = &game->undo_stack[game->undo_count++];
    step->row = (uint8_t)r;
    step->col = (uint8_t)c;
    step->old_value = cell->value;
    step->new_value = new_val;
    step->old_notes = cell->notes;
    step->new_notes = 0; // Placing value clears notes
  }

  cell->value = new_val;
  cell->notes = 0;
  game->moves_made++;

  if (new_val != 0 && new_val != cell->solution) {
    game->mistakes++;
  }

  game_check_conflicts(game);
  game_check_victory(game);
  return true;
}

bool game_toggle_note(GameState *game, int r, int c, int num) {
  if (r < 0 || r >= GRID_SIZE || c < 0 || c >= GRID_SIZE) return false;
  Cell *cell = &game->grid[r][c];
  if (cell->is_given || cell->value != 0) return false;
  if (num < 1 || num > 9) return false;

  uint16_t mask = (1 << num);
  uint16_t new_notes = cell->notes ^ mask;

  if (game->undo_count < MAX_UNDO_HISTORY) {
    UndoStep *step = &game->undo_stack[game->undo_count++];
    step->row = (uint8_t)r;
    step->col = (uint8_t)c;
    step->old_value = cell->value;
    step->new_value = cell->value;
    step->old_notes = cell->notes;
    step->new_notes = new_notes;
  }

  cell->notes = new_notes;
  game->moves_made++;
  return true;
}

bool game_clear_cell(GameState *game, int r, int c) {
  if (r < 0 || r >= GRID_SIZE || c < 0 || c >= GRID_SIZE) return false;
  Cell *cell = &game->grid[r][c];
  if (cell->is_given) return false;
  if (cell->value == 0 && cell->notes == 0) return false;

  if (game->undo_count < MAX_UNDO_HISTORY) {
    UndoStep *step = &game->undo_stack[game->undo_count++];
    step->row = (uint8_t)r;
    step->col = (uint8_t)c;
    step->old_value = cell->value;
    step->new_value = 0;
    step->old_notes = cell->notes;
    step->new_notes = 0;
  }

  cell->value = 0;
  cell->notes = 0;
  game->moves_made++;

  game_check_conflicts(game);
  return true;
}

bool game_undo(GameState *game) {
  if (game->undo_count <= 0) return false;

  UndoStep *step = &game->undo_stack[--game->undo_count];
  Cell *cell = &game->grid[step->row][step->col];

  cell->value = step->old_value;
  cell->notes = step->old_notes;
  game->cursor_r = step->row;
  game->cursor_c = step->col;

  game_check_conflicts(game);
  return true;
}

bool game_hint(GameState *game) {
  // If current cell is empty and non-given, hint that cell
  int target_r = -1;
  int target_c = -1;

  if (!game->grid[game->cursor_r][game->cursor_c].is_given &&
      game->grid[game->cursor_r][game->cursor_c].value != game->grid[game->cursor_r][game->cursor_c].solution) {
    target_r = game->cursor_r;
    target_c = game->cursor_c;
  } else {
    // Find next empty cell
    for (int r = 0; r < GRID_SIZE && target_r == -1; r++) {
      for (int c = 0; c < GRID_SIZE; c++) {
        if (!game->grid[r][c].is_given && game->grid[r][c].value != game->grid[r][c].solution) {
          target_r = r;
          target_c = c;
          break;
        }
      }
    }
  }

  if (target_r == -1) return false;

  Cell *cell = &game->grid[target_r][target_c];

  if (game->undo_count < MAX_UNDO_HISTORY) {
    UndoStep *step = &game->undo_stack[game->undo_count++];
    step->row = (uint8_t)target_r;
    step->col = (uint8_t)target_c;
    step->old_value = cell->value;
    step->new_value = cell->solution;
    step->old_notes = cell->notes;
    step->new_notes = 0;
  }

  cell->value = cell->solution;
  cell->notes = 0;
  game->cursor_r = (uint8_t)target_r;
  game->cursor_c = (uint8_t)target_c;
  game->hints_used++;
  game->moves_made++;

  game_check_conflicts(game);
  game_check_victory(game);
  return true;
}

bool game_check_victory(const GameState *game) {
  for (int r = 0; r < GRID_SIZE; r++) {
    for (int c = 0; c < GRID_SIZE; c++) {
      if (game->grid[r][c].value == 0 || game->grid[r][c].is_error ||
          game->grid[r][c].value != game->grid[r][c].solution) {
        return false;
      }
    }
  }

  ((GameState *)game)->state = STATE_VICTORY;
  return true;
}

uint16_t game_get_valid_candidates(const GameState *game, int r, int c) {
  uint16_t mask = 0;
  for (uint8_t v = 1; v <= 9; v++) {
    bool ok = true;
    for (int i = 0; i < GRID_SIZE && ok; i++) {
      if (i != c && game->grid[r][i].value == v) ok = false;
      if (i != r && game->grid[i][c].value == v) ok = false;
    }

    int start_r = (r / BOX_SIZE) * BOX_SIZE;
    int start_c = (c / BOX_SIZE) * BOX_SIZE;
    for (int br = 0; br < BOX_SIZE && ok; br++) {
      for (int bc = 0; bc < BOX_SIZE && ok; bc++) {
        int tr = start_r + br;
        int tc = start_c + bc;
        if ((tr != r || tc != c) && game->grid[tr][tc].value == v) {
          ok = false;
        }
      }
    }

    if (ok) {
      mask |= (1 << v);
    }
  }
  return mask;
}

void game_handle_action(GameState *game, GameAction action, unsigned long long current_time) {
  if (game->start_time_ms == 0 && action != ACT_QUIT) {
    game->start_time_ms = current_time;
  }

  if (action == ACT_QUIT) {
    if (game->state != STATE_VICTORY) {
      game_save(game, current_time);
    }
    game->state = STATE_EXIT;
    return;
  }

  if (action == ACT_PAUSE) {
    if (game->state == STATE_PLAYING) {
      game->state = STATE_PAUSED;
      game->pause_start_ms = current_time;
    } else if (game->state == STATE_PAUSED) {
      game->state = STATE_PLAYING;
      game->elapsed_paused_ms += (current_time - game->pause_start_ms);
    }
    return;
  }

  if (game->state == STATE_PAUSED) {
    return; // Ignore controls while paused
  }

  if (action == ACT_RESTART) {
    game_delete_save();
    game_init(game, game->difficulty);
    game->start_time_ms = current_time;
    return;
  }

  if (action == ACT_NEXT_DIFF) {
    DifficultyLevel next_d = (DifficultyLevel)((game->difficulty + 1) % DIFF_COUNT);
    game_delete_save();
    game_init(game, next_d);
    game->start_time_ms = current_time;
    return;
  }

  if (game->state == STATE_VICTORY) {
    return; // Only restart, change difficulty, or quit
  }

  switch (action) {
    case ACT_UP:
      if (game->cursor_r > 0) game->cursor_r--;
      break;

    case ACT_DOWN:
      if (game->cursor_r < GRID_SIZE - 1) game->cursor_r++;
      break;

    case ACT_LEFT:
      if (game->cursor_c > 0) game->cursor_c--;
      break;

    case ACT_RIGHT:
      if (game->cursor_c < GRID_SIZE - 1) game->cursor_c++;
      break;

    case ACT_NUM_1:
    case ACT_NUM_2:
    case ACT_NUM_3:
    case ACT_NUM_4:
    case ACT_NUM_5:
    case ACT_NUM_6:
    case ACT_NUM_7:
    case ACT_NUM_8:
    case ACT_NUM_9: {
      int digit = (int)(action - ACT_NUM_1 + 1);
      if (game->note_mode) {
        game_toggle_note(game, game->cursor_r, game->cursor_c, digit);
      } else {
        game_set_cell(game, game->cursor_r, game->cursor_c, digit);
      }
      break;
    }

    case ACT_CLEAR:
      game_clear_cell(game, game->cursor_r, game->cursor_c);
      break;

    case ACT_TOGGLE_NOTE_MODE:
      game->note_mode = !game->note_mode;
      break;

    case ACT_UNDO:
      game_undo(game);
      break;

    case ACT_HINT:
      game_hint(game);
      break;

    case ACT_TOGGLE_ERRORS:
      game->show_errors = !game->show_errors;
      break;

    default:
      break;
  }

  if (game->state == STATE_VICTORY) {
    game_delete_save();
  } else {
    game_save(game, current_time);
  }
}

void game_update(GameState *game, unsigned long long current_time) {
  (void)game;
  (void)current_time;
}
