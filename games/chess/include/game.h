#ifndef CHESS_GAME_H
#define CHESS_GAME_H

#include <stdbool.h>
#include <stddef.h>
#include "config.h"

typedef struct {
  PieceType type;
  PieceColor color;
} Piece;

typedef struct {
  int from_r;
  int from_c;
  int to_r;
  int to_c;
  PieceType promotion;
  bool is_castling;
  bool is_en_passant;
  Piece captured;
  int score_guess;
} ChessMove;

typedef struct {
  Piece board[BOARD_SIZE][BOARD_SIZE];
  PieceColor turn;
  bool castle_wk;
  bool castle_wq;
  bool castle_bk;
  bool castle_bq;
  int ep_r;
  int ep_c;
  int halfmove_clock;
  int fullmove_number;
  ChessMove move_made;
  char san[16];
  PieceType captured_by_white[32];
  int cap_white_count;
  PieceType captured_by_black[32];
  int cap_black_count;
  ChessMove last_move;
  bool has_last_move;
} BoardUndoState;

typedef struct {
  Piece board[BOARD_SIZE][BOARD_SIZE];
  PieceColor turn;

  /* Castling availability */
  bool castle_wk;
  bool castle_wq;
  bool castle_bk;
  bool castle_bq;

  /* En passant target square (-1 if none) */
  int ep_r;
  int ep_c;

  int halfmove_clock;
  int fullmove_number;

  GameStatus status;
  PieceColor winner;
  AIDifficulty difficulty;
  PieceColor human_color;
  bool board_flipped;

  /* UI Cursor & Selection */
  int cursor_r;
  int cursor_c;
  int selected_r;
  int selected_c;

  /* Legal moves cache for current turn */
  ChessMove legal_moves[MAX_MOVES];
  int legal_move_count;

  /* Pawn Promotion modal state */
  bool awaiting_promotion;
  int promo_from_r;
  int promo_from_c;
  int promo_to_r;
  int promo_to_c;

  /* Move history & undo stack */
  BoardUndoState history[MAX_HISTORY];
  int history_count;

  /* Captured pieces lists */
  PieceType captured_by_white[32];
  int cap_white_count;
  PieceType captured_by_black[32];
  int cap_black_count;

  /* Last move highlight */
  ChessMove last_move;
  bool has_last_move;

  char status_msg[80];

  /* Persistent stats */
  int wins_player;
  int games_played;
} GameState;

/* Core initialization & state management */
void game_init(GameState *g, AIDifficulty diff);
void game_reset_board(GameState *g);
void game_clear_board(GameState *g);
void game_refresh_legal_moves(GameState *g);
void game_save_scores(const GameState *g);

/* Move generation & rule checks */
bool game_is_square_attacked(const GameState *g, int r, int c, PieceColor by_color);
bool game_is_in_check(const GameState *g, PieceColor color);
int game_generate_pseudo_moves(const GameState *g, ChessMove *moves);
int game_generate_legal_moves(const GameState *g, ChessMove *moves);
bool game_make_move(GameState *g, ChessMove m, bool record_history);
bool game_undo(GameState *g);

/* AI & Evaluation */
int game_evaluate_board(const GameState *g);
ChessMove game_ai_best_move(const GameState *g, int depth);
bool game_ai_step(GameState *g);
bool game_get_hint(const GameState *g, char *out_buf, size_t buf_size);
int game_material_advantage_white(const GameState *g);

/* Interactive UI helpers */
void game_move_cursor(GameState *g, int dr, int dc);
void game_select_square(GameState *g, int r, int c);
void game_cancel_selection(GameState *g);
void game_complete_promotion(GameState *g, PieceType promo_type);
void game_cycle_difficulty(GameState *g);
const char *game_difficulty_name(AIDifficulty diff);
const char *game_piece_symbol(Piece p);
char game_piece_letter(PieceType t);

#endif /* CHESS_GAME_H */
