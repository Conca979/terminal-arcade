#ifndef SUDOKU_GAME_H
#define SUDOKU_GAME_H

#include <stdbool.h>
#include <stdint.h>
#include "config.h"

typedef enum {
  ACT_NONE,
  ACT_UP,
  ACT_DOWN,
  ACT_LEFT,
  ACT_RIGHT,
  ACT_NUM_1,
  ACT_NUM_2,
  ACT_NUM_3,
  ACT_NUM_4,
  ACT_NUM_5,
  ACT_NUM_6,
  ACT_NUM_7,
  ACT_NUM_8,
  ACT_NUM_9,
  ACT_CLEAR,
  ACT_TOGGLE_NOTE_MODE,
  ACT_UNDO,
  ACT_HINT,
  ACT_TOGGLE_ERRORS,
  ACT_NEXT_DIFF,
  ACT_RESTART,
  ACT_PAUSE,
  ACT_QUIT
} GameAction;

typedef enum {
  STATE_PLAYING,
  STATE_PAUSED,
  STATE_VICTORY,
  STATE_EXIT
} PlayState;

typedef struct {
  uint8_t row;
  uint8_t col;
  uint8_t old_value;
  uint8_t new_value;
  uint16_t old_notes;
  uint16_t new_notes;
} UndoStep;

typedef struct {
  uint8_t value;        // 0 = empty, 1..9
  uint8_t solution;     // Solved target value
  bool is_given;        // Initial puzzle clue (cannot modify)
  bool is_error;        // Conflict with row/col/box or wrong value
  uint16_t notes;       // Bitmask: bit 1..9 set if candidate note active
} Cell;

typedef struct {
  Cell grid[GRID_SIZE][GRID_SIZE];
  uint8_t cursor_r;
  uint8_t cursor_c;
  DifficultyLevel difficulty;
  PlayState state;
  bool note_mode;            // If true, 1-9 toggles notes instead of placing value
  bool show_errors;          // Real-time conflict highlighting
  int mistakes;              // Mistake counter
  int hints_used;            // Hints used counter
  int moves_made;            // Move count
  unsigned long long start_time_ms;
  unsigned long long elapsed_paused_ms;
  unsigned long long pause_start_ms;
  unsigned long long saved_elapsed_ms;

  UndoStep undo_stack[MAX_UNDO_HISTORY];
  int undo_count;

  int digit_counts[10];      // 0..9: count of times digit is placed on board
  int remaining_counts[10];  // 1..9: remaining needed (9 - digit_counts)
} GameState;

void game_init(GameState *game, DifficultyLevel diff);
void game_reset_board(GameState *game);
void game_generate_puzzle(GameState *game, DifficultyLevel diff);
void game_handle_action(GameState *game, GameAction action, unsigned long long current_time);
void game_update(GameState *game, unsigned long long current_time);

bool game_set_cell(GameState *game, int r, int c, int val);
bool game_toggle_note(GameState *game, int r, int c, int num);
bool game_clear_cell(GameState *game, int r, int c);
bool game_undo(GameState *game);
bool game_hint(GameState *game);
void game_check_conflicts(GameState *game);
bool game_check_victory(const GameState *game);

int game_get_elapsed_seconds(const GameState *game, unsigned long long current_time);
uint16_t game_get_valid_candidates(const GameState *game, int r, int c);

void game_save(const GameState *game, unsigned long long current_time);
bool game_load(GameState *game);
void game_delete_save(void);

#endif // SUDOKU_GAME_H
