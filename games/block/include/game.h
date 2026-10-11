#ifndef BLOCK_GAME_H
#define BLOCK_GAME_H

#include <stdbool.h>
#include <stdint.h>
#include "config.h"

typedef enum {
  ACT_NONE,
  ACT_UP,
  ACT_DOWN,
  ACT_LEFT,
  ACT_RIGHT,
  ACT_SELECT_1,
  ACT_SELECT_2,
  ACT_SELECT_3,
  ACT_DESELECT,
  ACT_PLACE,
  ACT_MOUSE_MOVE,
  ACT_MOUSE_CLICK,
  ACT_RESTART,
  ACT_QUIT
} GameAction;

typedef struct {
  int w;
  int h;
  int block_count;
  uint8_t mask[MAX_SHAPE_DIM][MAX_SHAPE_DIM];
  int color_id;
} Shape;

typedef struct {
  Shape shape;
  bool available;
} TrayPiece;

typedef struct {
  uint8_t grid[GRID_SIZE][GRID_SIZE];
  int cursor_r;
  int cursor_c;

  TrayPiece tray[MAX_PIECES_TRAY];
  int selected_piece; // -1 if none selected, 0..2 for tray slots

  int score;
  int high_score;
  int lines_cleared;
  int streak;
  int blocks_placed;

  bool game_over;
  bool exit_requested;
  bool resumed_game;

  char combo_banner[64];
  int combo_timer;

  int mouse_term_x;
  int mouse_term_y;
} GameState;

void game_init(GameState *game);
void game_reset(GameState *game);
void game_save(const GameState *game);
bool game_load(GameState *game);
void game_delete_save(void);

void game_deal_pieces(GameState *game);
bool game_can_place(const GameState *game, int tray_idx, int r, int c);
bool game_place_piece(GameState *game, int tray_idx, int r, int c);
bool game_has_any_valid_move(const GameState *game);

void game_handle_action(GameState *game, GameAction action, int param1, int param2, unsigned long long current_time);
void game_update(GameState *game, unsigned long long current_time);

#endif // BLOCK_GAME_H
