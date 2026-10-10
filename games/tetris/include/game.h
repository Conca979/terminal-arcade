#ifndef GAME_H
#define GAME_H

#include "config.h"

typedef struct {
  int x;
  int y;
} Point;

typedef struct {
  TetrominoType type;
  int rotation; // 0, 1, 2, 3
  int x;        // Board x position (0 to 9)
  int y;        // Board y position (0 to 21)
} ActivePiece;

typedef struct {
  TetrominoType board[TOTAL_HEIGHT][BOARD_WIDTH];
  ActivePiece current;
  TetrominoType hold_piece;
  bool can_hold;
  TetrominoType next_queue[14];
  int next_queue_count;

  // Stats
  int score;
  int high_score;
  int lines_cleared;
  int level;

  // State & Timing
  PlayState state;
  unsigned long long last_fall_time;
  unsigned long long lock_start_time;
  bool is_locking;
  int lock_moves_count;

  // Line clear animation
  int clearing_rows[4];
  int clearing_row_count;
  unsigned long long line_clear_start_time;

  // Bag randomizer
  TetrominoType bag[7];
  int bag_index;
} GameState;

void game_init(GameState *game);
void game_reset(GameState *game);
void game_update(GameState *game, unsigned long long current_time);
void game_handle_action(GameState *game, GameAction action, unsigned long long current_time);
bool game_check_collision(const GameState *game, const ActivePiece *piece);
int game_get_ghost_y(const GameState *game);
void game_get_piece_blocks(TetrominoType type, int rotation, Point out_blocks[4]);
int game_get_fall_interval_ms(int level);
void game_load_high_score(GameState *game);
void game_save_high_score(const GameState *game);

#endif /* GAME_H */
