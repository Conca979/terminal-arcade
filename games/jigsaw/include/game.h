#ifndef JIGSAW_GAME_H
#define JIGSAW_GAME_H

#include "config.h"
#include <stdbool.h>

#define MAX_UNDO 128

typedef struct {
  int board[GRID_SIZE][GRID_SIZE];
  bool tray_present[TOTAL_PIECES];
  int holding_piece;
  int moves_count;
  int snapped_count;
} JigsawUndoState;

typedef struct {
  int board[GRID_SIZE][GRID_SIZE];      /* Piece ID placed at (r, c), or -1 if empty */
  int tray_order[TOTAL_PIECES];         /* Shuffled display order of pieces in tray */
  bool tray_present[TOTAL_PIECES];      /* Whether piece_id is in tray */
  int holding_piece;                    /* Piece ID currently picked up, or -1 */

  int cursor_board_r;                   /* 0..3 */
  int cursor_board_c;                   /* 0..3 */
  int cursor_tray_idx;                  /* 0..15 */
  FocusArea focus;                      /* FOCUS_BOARD or FOCUS_TRAY */

  int current_theme;                    /* 0..THEME_COUNT-1 */
  bool show_preview;                    /* Show full art preview reference */

  int hint_piece_id;                    /* Highlighted piece, or -1 */
  int hint_target_r;                    /* Highlighted target slot row, or -1 */
  int hint_target_c;                    /* Highlighted target slot col, or -1 */
  int hint_timer_ticks;                 /* Ticks remaining for hint flash */

  int moves_count;                      /* Number of placements made */
  int snapped_count;                    /* Number of correctly placed pieces */
  bool is_solved;                       /* True when all 16 pieces snapped */
  double elapsed_seconds;               /* Stopwatch elapsed time */

  int puzzles_solved;                   /* Lifetime puzzles solved */
  int best_moves;                       /* Best move record for current theme */
  int best_time;                        /* Best time record for current theme in seconds */

  char status_msg[96];                  /* Bottom message / HUD feedback */

  JigsawUndoState undo_stack[MAX_UNDO]; /* Deep undo history */
  int undo_count;
} GameState;

void game_init(GameState *g, int theme_idx);
void game_reset(GameState *g, int theme_idx);
void game_next_theme(GameState *g);

bool game_pick_from_tray(GameState *g, int piece_id);
bool game_place_on_board(GameState *g, int r, int c);
bool game_return_holding_to_tray(GameState *g);
bool game_return_slot_to_tray(GameState *g, int r, int c);
void game_interact_current_focus(GameState *g);

void game_toggle_focus(GameState *g);
void game_move_cursor(GameState *g, int dr, int dc);
void game_request_hint(GameState *g);
void game_toggle_preview(GameState *g);
bool game_undo(GameState *g);

void game_update(GameState *g, double dt);
void game_save_scores(const GameState *g);

#endif /* JIGSAW_GAME_H */
