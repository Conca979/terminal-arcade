#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "config.h"
#include "game.h"
#include "render.h"
#include "scores.h"

static void test_initial_setup_and_opening_moves(void) {
  GameState g;
  int saved_wins = arcade_score_get("chess", "wins");
  int saved_played = arcade_score_get("chess", "games_played");

  game_init(&g, AI_MEDIUM);
  arcade_score_set("chess", "wins", saved_wins);
  arcade_score_set("chess", "games_played", saved_played);

  /* Standard starting position has 20 legal moves (16 pawn moves + 4 knight moves) */
  assert(g.turn == COLOR_WHITE);
  assert(g.legal_move_count == 20);
  assert(g.status == STATUS_PLAYING);
  assert(game_material_advantage_white(&g) == 0);
  assert(game_evaluate_board(&g) == 0);

  printf("[PASS] test_initial_setup_and_opening_moves\n");
}

static void test_pins_and_check_evasion(void) {
  GameState g;
  game_clear_board(&g);

  /* White King at e1 (7,4), White Rook at e2 (6,4), Black Rook at e8 (0,4), Black King at a8 (0,0) */
  g.board[7][4] = (Piece){ PIECE_KING, COLOR_WHITE };
  g.board[6][4] = (Piece){ PIECE_ROOK, COLOR_WHITE };
  g.board[0][4] = (Piece){ PIECE_ROOK, COLOR_BLACK };
  g.board[0][0] = (Piece){ PIECE_KING, COLOR_BLACK };
  g.turn = COLOR_WHITE;

  game_refresh_legal_moves(&g);

  /* The White Rook at e2 is pinned along the e-file: it can only move along col 4, never horizontally! */
  for (int i = 0; i < g.legal_move_count; i++) {
    ChessMove m = g.legal_moves[i];
    if (m.from_r == 6 && m.from_c == 4) {
      assert(m.to_c == 4);
    }
  }

  printf("[PASS] test_pins_and_check_evasion\n");
}

static void test_castling_rules(void) {
  GameState g;
  game_clear_board(&g);

  g.board[7][4] = (Piece){ PIECE_KING, COLOR_WHITE };
  g.board[7][0] = (Piece){ PIECE_ROOK, COLOR_WHITE };
  g.board[7][7] = (Piece){ PIECE_ROOK, COLOR_WHITE };
  g.board[0][4] = (Piece){ PIECE_KING, COLOR_BLACK };
  g.castle_wk = true;
  g.castle_wq = true;
  g.turn = COLOR_WHITE;

  game_refresh_legal_moves(&g);

  bool found_wk = false;
  bool found_wq = false;
  for (int i = 0; i < g.legal_move_count; i++) {
    if (g.legal_moves[i].is_castling && g.legal_moves[i].to_c == 6) found_wk = true;
    if (g.legal_moves[i].is_castling && g.legal_moves[i].to_c == 2) found_wq = true;
  }
  assert(found_wk && found_wq);

  /* Place a Black Rook attacking f1 (7,5) — Kingside castling must now be forbidden */
  g.board[2][5] = (Piece){ PIECE_ROOK, COLOR_BLACK };
  game_refresh_legal_moves(&g);

  found_wk = false;
  found_wq = false;
  for (int i = 0; i < g.legal_move_count; i++) {
    if (g.legal_moves[i].is_castling && g.legal_moves[i].to_c == 6) found_wk = true;
    if (g.legal_moves[i].is_castling && g.legal_moves[i].to_c == 2) found_wq = true;
  }
  assert(!found_wk && found_wq);

  /* Execute Queenside castling (e1 -> c1) and verify Rook jumped a1 -> d1 */
  ChessMove wq_move = { 7, 4, 7, 2, PIECE_NONE, true, false, { PIECE_NONE, COLOR_NONE }, 0 };
  bool ok = game_make_move(&g, wq_move, true);
  assert(ok);
  assert(g.board[7][2].type == PIECE_KING && g.board[7][2].color == COLOR_WHITE);
  assert(g.board[7][3].type == PIECE_ROOK && g.board[7][3].color == COLOR_WHITE);
  assert(g.board[7][0].type == PIECE_NONE);

  printf("[PASS] test_castling_rules\n");
}

static void test_en_passant_and_promotion(void) {
  GameState g;
  game_clear_board(&g);

  g.board[7][4] = (Piece){ PIECE_KING, COLOR_WHITE };
  g.board[0][4] = (Piece){ PIECE_KING, COLOR_BLACK };
  /* White pawn on e5 (3,4), Black pawn on d7 (1,3) */
  g.board[3][4] = (Piece){ PIECE_PAWN, COLOR_WHITE };
  g.board[1][3] = (Piece){ PIECE_PAWN, COLOR_BLACK };
  g.turn = COLOR_BLACK;
  game_refresh_legal_moves(&g);

  /* Black plays d7 -> d5 (double push) */
  ChessMove d5 = { 1, 3, 3, 3, PIECE_NONE, false, false, { PIECE_NONE, COLOR_NONE }, 0 };
  assert(game_make_move(&g, d5, true));
  assert(g.ep_r == 2 && g.ep_c == 3);

  /* White captures en passant: e5xd6 (3,4 -> 2,3) */
  ChessMove ep = { 3, 4, 2, 3, PIECE_NONE, false, true, { PIECE_PAWN, COLOR_BLACK }, 0 };
  assert(game_make_move(&g, ep, true));
  assert(g.board[2][3].type == PIECE_PAWN && g.board[2][3].color == COLOR_WHITE);
  assert(g.board[3][3].type == PIECE_NONE); /* Captured pawn on d5 is gone */

  /* Test Promotion via UI selection */
  game_clear_board(&g);
  g.board[7][4] = (Piece){ PIECE_KING, COLOR_WHITE };
  g.board[0][4] = (Piece){ PIECE_KING, COLOR_BLACK };
  g.board[1][0] = (Piece){ PIECE_PAWN, COLOR_WHITE };
  g.turn = COLOR_WHITE;
  game_refresh_legal_moves(&g);

  game_select_square(&g, 1, 0);
  game_select_square(&g, 0, 0);
  assert(g.awaiting_promotion);
  game_complete_promotion(&g, PIECE_KNIGHT);
  assert(!g.awaiting_promotion);
  assert(g.board[0][0].type == PIECE_KNIGHT && g.board[0][0].color == COLOR_WHITE);

  printf("[PASS] test_en_passant_and_promotion\n");
}

static void test_fools_mate_and_stalemate(void) {
  GameState g;
  game_reset_board(&g);
  g.difficulty = AI_TWO_PLAYER;

  /* Fool's Mate: 1. f3 e5 2. g4 Qh4# */
  ChessMove m1 = { 6, 5, 5, 5, PIECE_NONE, false, false, { 0 }, 0 }; /* f2-f3 */
  ChessMove m2 = { 1, 4, 3, 4, PIECE_NONE, false, false, { 0 }, 0 }; /* e7-e5 */
  ChessMove m3 = { 6, 6, 4, 6, PIECE_NONE, false, false, { 0 }, 0 }; /* g2-g4 */
  ChessMove m4 = { 0, 3, 4, 7, PIECE_NONE, false, false, { 0 }, 0 }; /* Qd8-h4# */

  assert(game_make_move(&g, m1, true));
  assert(game_make_move(&g, m2, true));
  assert(game_make_move(&g, m3, true));
  assert(game_make_move(&g, m4, true));

  assert(g.status == STATUS_CHECKMATE);
  assert(g.winner == COLOR_BLACK);
  assert(strcmp(g.history[3].san, "Qh4#") == 0);

  /* Undo should restore playing state */
  assert(game_undo(&g));
  assert(g.status == STATUS_PLAYING);

  /* Stalemate position: Black King at a8 (0,0), White Queen at b6 (2,1), White King at c6 (2,2) */
  game_clear_board(&g);
  g.board[0][0] = (Piece){ PIECE_KING,  COLOR_BLACK };
  g.board[2][1] = (Piece){ PIECE_QUEEN, COLOR_WHITE };
  g.board[2][2] = (Piece){ PIECE_KING,  COLOR_WHITE };
  g.turn = COLOR_BLACK;
  game_refresh_legal_moves(&g);
  assert(g.status == STATUS_STALEMATE);

  printf("[PASS] test_fools_mate_and_stalemate\n");
}

static void test_ai_tactics_and_mate_in_one(void) {
  GameState g;
  game_clear_board(&g);
  g.difficulty = AI_MEDIUM;
  g.human_color = COLOR_WHITE;

  /* 1. AI (Black) has mate in 1: White King at h1 (7,7), pawns at g2 (6,6), h2 (6,7);
     Black Rook at d8 (0,3) can play Rd1# (7,3) */
  g.board[7][7] = (Piece){ PIECE_KING, COLOR_WHITE };
  g.board[6][6] = (Piece){ PIECE_PAWN, COLOR_WHITE };
  g.board[6][7] = (Piece){ PIECE_PAWN, COLOR_WHITE };
  g.board[0][0] = (Piece){ PIECE_KING, COLOR_BLACK };
  g.board[0][3] = (Piece){ PIECE_ROOK, COLOR_BLACK };
  g.turn = COLOR_BLACK;
  game_refresh_legal_moves(&g);

  ChessMove mate_move = game_ai_best_move(&g, 2);
  assert(mate_move.from_r == 0 && mate_move.from_c == 3);
  assert(mate_move.to_r == 7 && mate_move.to_c == 3);

  /* 2. Quiescence anti-blunder check:
     Black Queen at d5 (3,3) can capture undefended White Knight at g5 (3,6),
     OR capture defended White Pawn at d4 (4,3) protected by White Pawn at e3 (5,4).
     AI must capture the undefended Knight at g5 rather than losing the Queen on d4! */
  game_clear_board(&g);
  g.board[7][4] = (Piece){ PIECE_KING,   COLOR_WHITE };
  g.board[0][4] = (Piece){ PIECE_KING,   COLOR_BLACK };
  g.board[3][3] = (Piece){ PIECE_QUEEN,  COLOR_BLACK };
  g.board[3][6] = (Piece){ PIECE_KNIGHT, COLOR_WHITE }; /* Free knight! */
  g.board[4][3] = (Piece){ PIECE_PAWN,   COLOR_WHITE };
  g.board[5][4] = (Piece){ PIECE_PAWN,   COLOR_WHITE }; /* Protects d4 */
  g.turn = COLOR_BLACK;
  game_refresh_legal_moves(&g);

  ChessMove smart = game_ai_best_move(&g, 2);
  assert(smart.from_r == 3 && smart.from_c == 3);
  assert(smart.to_r == 3 && smart.to_c == 6);

  printf("[PASS] test_ai_tactics_and_mate_in_one\n");
}

static void test_render_and_mouse_hit(void) {
  GameState g;
  int saved_wins = arcade_score_get("chess", "wins");
  int saved_played = arcade_score_get("chess", "games_played");

  game_init(&g, AI_MEDIUM);
  arcade_score_set("chess", "wins", saved_wins);
  arcade_score_set("chess", "games_played", saved_played);

  int r = -1;
  int c = -1;
  /* Top-left square a8 (0,0) is at x=7..11, y=5 */
  assert(render_get_square_at_pos(&g, 8, 5, &r, &c));
  assert(r == 0 && c == 0);

  /* Bottom-right square h1 (7,7) is at x=49..53, y=19 */
  assert(render_get_square_at_pos(&g, 50, 19, &r, &c));
  assert(r == 7 && c == 7);

  /* When board is flipped, top-left becomes h1 (7,7) */
  g.board_flipped = true;
  assert(render_get_square_at_pos(&g, 8, 5, &r, &c));
  assert(r == 7 && c == 7);
  g.board_flipped = false;

  volatile int canary = 0x5A5A5A5A;
  render_game(&g);
  assert(canary == 0x5A5A5A5A);

  printf("[PASS] test_render_and_mouse_hit\n");
}

int main(void) {
  printf("Running Chess CLI unit tests...\n\n");

  test_initial_setup_and_opening_moves();
  test_pins_and_check_evasion();
  test_castling_rules();
  test_en_passant_and_promotion();
  test_fools_mate_and_stalemate();
  test_ai_tactics_and_mate_in_one();
  test_render_and_mouse_hit();

  printf("\nAll Chess CLI unit tests PASSED successfully!\n");
  return 0;
}
