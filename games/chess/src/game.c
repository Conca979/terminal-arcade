#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scores.h"

#define INF_SCORE 1000000
#define MATE_SCORE 500000

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
} LightUndo;

static PieceColor opposite_color(PieceColor c) {
  if (c == COLOR_WHITE) return COLOR_BLACK;
  if (c == COLOR_BLACK) return COLOR_WHITE;
  return COLOR_NONE;
}

static bool in_bounds(int r, int c) {
  return (r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE);
}

const char *game_difficulty_name(AIDifficulty diff) {
  switch (diff) {
    case AI_EASY:       return "AI Easy";
    case AI_MEDIUM:     return "AI Medium";
    case AI_HARD:       return "AI Hard";
    case AI_TWO_PLAYER: return "2-Player";
    default:            return "Unknown";
  }
}

const char *game_piece_symbol(Piece p) {
  if (p.color == COLOR_WHITE) {
    switch (p.type) {
      case PIECE_KING:   return "♔";
      case PIECE_QUEEN:  return "♕";
      case PIECE_ROOK:   return "♖";
      case PIECE_BISHOP: return "♗";
      case PIECE_KNIGHT: return "♘";
      case PIECE_PAWN:   return "♙";
      default:           return " ";
    }
  } else if (p.color == COLOR_BLACK) {
    switch (p.type) {
      case PIECE_KING:   return "♚";
      case PIECE_QUEEN:  return "♛";
      case PIECE_ROOK:   return "♜";
      case PIECE_BISHOP: return "♝";
      case PIECE_KNIGHT: return "♞";
      case PIECE_PAWN:   return "♟";
      default:           return " ";
    }
  }
  return " ";
}

char game_piece_letter(PieceType t) {
  switch (t) {
    case PIECE_KING:   return 'K';
    case PIECE_QUEEN:  return 'Q';
    case PIECE_ROOK:   return 'R';
    case PIECE_BISHOP: return 'B';
    case PIECE_KNIGHT: return 'N';
    case PIECE_PAWN:   return 'P';
    default:           return ' ';
  }
}

static void save_light_undo(const GameState *g, LightUndo *u) {
  memcpy(u->board, g->board, sizeof(g->board));
  u->turn = g->turn;
  u->castle_wk = g->castle_wk;
  u->castle_wq = g->castle_wq;
  u->castle_bk = g->castle_bk;
  u->castle_bq = g->castle_bq;
  u->ep_r = g->ep_r;
  u->ep_c = g->ep_c;
  u->halfmove_clock = g->halfmove_clock;
}

static void restore_light_undo(GameState *g, const LightUndo *u) {
  memcpy(g->board, u->board, sizeof(g->board));
  g->turn = u->turn;
  g->castle_wk = u->castle_wk;
  g->castle_wq = u->castle_wq;
  g->castle_bk = u->castle_bk;
  g->castle_bq = u->castle_bq;
  g->ep_r = u->ep_r;
  g->ep_c = u->ep_c;
  g->halfmove_clock = u->halfmove_clock;
}

static void apply_move_raw(GameState *g, ChessMove m) {
  Piece moving = g->board[m.from_r][m.from_c];

  g->ep_r = -1;
  g->ep_c = -1;

  if (moving.type == PIECE_PAWN || m.captured.type != PIECE_NONE) {
    g->halfmove_clock = 0;
  } else {
    g->halfmove_clock++;
  }

  /* En passant capture removes the pawn on (from_r, to_c) */
  if (m.is_en_passant) {
    g->board[m.from_r][m.to_c] = (Piece){ PIECE_NONE, COLOR_NONE };
  }

  /* Castling moves the corresponding rook */
  if (m.is_castling) {
    if (m.to_c == 6) { /* Kingside */
      g->board[m.to_r][5] = g->board[m.to_r][7];
      g->board[m.to_r][7] = (Piece){ PIECE_NONE, COLOR_NONE };
    } else if (m.to_c == 2) { /* Queenside */
      g->board[m.to_r][3] = g->board[m.to_r][0];
      g->board[m.to_r][0] = (Piece){ PIECE_NONE, COLOR_NONE };
    }
  }

  /* Double pawn push sets en passant square */
  if (moving.type == PIECE_PAWN && abs(m.to_r - m.from_r) == 2) {
    g->ep_r = (m.from_r + m.to_r) / 2;
    g->ep_c = m.from_c;
  }

  /* Update castling rights if King or Rook moves or is captured */
  if (moving.type == PIECE_KING) {
    if (moving.color == COLOR_WHITE) {
      g->castle_wk = false;
      g->castle_wq = false;
    } else {
      g->castle_bk = false;
      g->castle_bq = false;
    }
  }
  if (m.from_r == 7 && m.from_c == 0) g->castle_wq = false;
  if (m.from_r == 7 && m.from_c == 7) g->castle_wk = false;
  if (m.from_r == 0 && m.from_c == 0) g->castle_bq = false;
  if (m.from_r == 0 && m.from_c == 7) g->castle_bk = false;

  if (m.to_r == 7 && m.to_c == 0) g->castle_wq = false;
  if (m.to_r == 7 && m.to_c == 7) g->castle_wk = false;
  if (m.to_r == 0 && m.to_c == 0) g->castle_bq = false;
  if (m.to_r == 0 && m.to_c == 7) g->castle_bk = false;

  /* Move the piece (and promote if applicable) */
  if (m.promotion != PIECE_NONE) {
    moving.type = m.promotion;
  }
  g->board[m.to_r][m.to_c] = moving;
  g->board[m.from_r][m.from_c] = (Piece){ PIECE_NONE, COLOR_NONE };

  if (g->turn == COLOR_BLACK) {
    g->fullmove_number++;
  }
  g->turn = opposite_color(g->turn);
}

bool game_is_square_attacked(const GameState *g, int r, int c, PieceColor by_color) {
  /* 1. Attacked by pawns */
  int pawn_row = (by_color == COLOR_WHITE) ? (r + 1) : (r - 1);
  if (in_bounds(pawn_row, c - 1)) {
    Piece p = g->board[pawn_row][c - 1];
    if (p.color == by_color && p.type == PIECE_PAWN) return true;
  }
  if (in_bounds(pawn_row, c + 1)) {
    Piece p = g->board[pawn_row][c + 1];
    if (p.color == by_color && p.type == PIECE_PAWN) return true;
  }

  /* 2. Attacked by knights */
  static const int kn_dr[8] = { -2, -2, -1, -1,  1,  1,  2,  2 };
  static const int kn_dc[8] = { -1,  1, -2,  2, -2,  2, -1,  1 };
  for (int i = 0; i < 8; i++) {
    int nr = r + kn_dr[i];
    int nc = c + kn_dc[i];
    if (in_bounds(nr, nc)) {
      Piece p = g->board[nr][nc];
      if (p.color == by_color && p.type == PIECE_KNIGHT) return true;
    }
  }

  /* 3. Attacked by king */
  for (int dr = -1; dr <= 1; dr++) {
    for (int dc = -1; dc <= 1; dc++) {
      if (dr == 0 && dc == 0) continue;
      int nr = r + dr;
      int nc = c + dc;
      if (in_bounds(nr, nc)) {
        Piece p = g->board[nr][nc];
        if (p.color == by_color && p.type == PIECE_KING) return true;
      }
    }
  }

  /* 4. Orthogonal rays (Rook / Queen) */
  static const int ortho_dr[4] = { -1, 1,  0, 0 };
  static const int ortho_dc[4] = {  0, 0, -1, 1 };
  for (int d = 0; d < 4; d++) {
    int nr = r + ortho_dr[d];
    int nc = c + ortho_dc[d];
    while (in_bounds(nr, nc)) {
      Piece p = g->board[nr][nc];
      if (p.type != PIECE_NONE) {
        if (p.color == by_color && (p.type == PIECE_ROOK || p.type == PIECE_QUEEN)) {
          return true;
        }
        break;
      }
      nr += ortho_dr[d];
      nc += ortho_dc[d];
    }
  }

  /* 5. Diagonal rays (Bishop / Queen) */
  static const int diag_dr[4] = { -1, -1,  1, 1 };
  static const int diag_dc[4] = { -1,  1, -1, 1 };
  for (int d = 0; d < 4; d++) {
    int nr = r + diag_dr[d];
    int nc = c + diag_dc[d];
    while (in_bounds(nr, nc)) {
      Piece p = g->board[nr][nc];
      if (p.type != PIECE_NONE) {
        if (p.color == by_color && (p.type == PIECE_BISHOP || p.type == PIECE_QUEEN)) {
          return true;
        }
        break;
      }
      nr += diag_dr[d];
      nc += diag_dc[d];
    }
  }

  return false;
}

bool game_is_in_check(const GameState *g, PieceColor color) {
  int kr = -1;
  int kc = -1;
  for (int r = 0; r < BOARD_SIZE; r++) {
    for (int c = 0; c < BOARD_SIZE; c++) {
      if (g->board[r][c].color == color && g->board[r][c].type == PIECE_KING) {
        kr = r;
        kc = c;
        break;
      }
    }
    if (kr != -1) break;
  }
  if (kr == -1) return false;
  return game_is_square_attacked(g, kr, kc, opposite_color(color));
}

static void add_move(ChessMove *moves, int *count, int fr, int fc, int tr, int tc,
                     PieceType promo, bool castling, bool ep, Piece cap) {
  if (*count >= MAX_MOVES) return;
  ChessMove *m = &moves[(*count)++];
  m->from_r = fr;
  m->from_c = fc;
  m->to_r = tr;
  m->to_c = tc;
  m->promotion = promo;
  m->is_castling = castling;
  m->is_en_passant = ep;
  m->captured = cap;

  /* MVV-LVA score guess for move ordering */
  m->score_guess = 0;
  if (cap.type != PIECE_NONE) {
    m->score_guess += 10 * PIECE_VALUES[cap.type];
  }
  if (promo != PIECE_NONE) {
    m->score_guess += PIECE_VALUES[promo];
  }
}

static void add_pawn_move_with_promos(ChessMove *moves, int *count, int fr, int fc,
                                      int tr, int tc, Piece cap) {
  if (tr == 0 || tr == 7) {
    add_move(moves, count, fr, fc, tr, tc, PIECE_QUEEN,  false, false, cap);
    add_move(moves, count, fr, fc, tr, tc, PIECE_ROOK,   false, false, cap);
    add_move(moves, count, fr, fc, tr, tc, PIECE_BISHOP, false, false, cap);
    add_move(moves, count, fr, fc, tr, tc, PIECE_KNIGHT, false, false, cap);
  } else {
    add_move(moves, count, fr, fc, tr, tc, PIECE_NONE, false, false, cap);
  }
}

int game_generate_pseudo_moves(const GameState *g, ChessMove *moves) {
  int count = 0;
  PieceColor us = g->turn;
  PieceColor them = opposite_color(us);
  Piece none_piece = { PIECE_NONE, COLOR_NONE };

  for (int r = 0; r < BOARD_SIZE; r++) {
    for (int c = 0; c < BOARD_SIZE; c++) {
      Piece p = g->board[r][c];
      if (p.color != us) continue;

      if (p.type == PIECE_PAWN) {
        int dir = (us == COLOR_WHITE) ? -1 : 1;
        int start_row = (us == COLOR_WHITE) ? 6 : 1;
        int nr = r + dir;

        /* 1-step forward */
        if (in_bounds(nr, c) && g->board[nr][c].type == PIECE_NONE) {
          add_pawn_move_with_promos(moves, &count, r, c, nr, c, none_piece);
          /* 2-step forward from starting rank */
          int nnr = r + 2 * dir;
          if (r == start_row && in_bounds(nnr, c) && g->board[nnr][c].type == PIECE_NONE) {
            add_move(moves, &count, r, c, nnr, c, PIECE_NONE, false, false, none_piece);
          }
        }

        /* Diagonal captures & En Passant */
        for (int dc = -1; dc <= 1; dc += 2) {
          int nc = c + dc;
          if (!in_bounds(nr, nc)) continue;
          Piece target = g->board[nr][nc];
          if (target.color == them) {
            add_pawn_move_with_promos(moves, &count, r, c, nr, nc, target);
          } else if (nr == g->ep_r && nc == g->ep_c) {
            Piece ep_cap = { PIECE_PAWN, them };
            add_move(moves, &count, r, c, nr, nc, PIECE_NONE, false, true, ep_cap);
          }
        }
      } else if (p.type == PIECE_KNIGHT) {
        static const int kn_dr[8] = { -2, -2, -1, -1,  1,  1,  2,  2 };
        static const int kn_dc[8] = { -1,  1, -2,  2, -2,  2, -1,  1 };
        for (int i = 0; i < 8; i++) {
          int nr = r + kn_dr[i];
          int nc = c + kn_dc[i];
          if (in_bounds(nr, nc) && g->board[nr][nc].color != us) {
            add_move(moves, &count, r, c, nr, nc, PIECE_NONE, false, false, g->board[nr][nc]);
          }
        }
      } else if (p.type == PIECE_KING) {
        for (int dr = -1; dr <= 1; dr++) {
          for (int dc = -1; dc <= 1; dc++) {
            if (dr == 0 && dc == 0) continue;
            int nr = r + dr;
            int nc = c + dc;
            if (in_bounds(nr, nc) && g->board[nr][nc].color != us) {
              add_move(moves, &count, r, c, nr, nc, PIECE_NONE, false, false, g->board[nr][nc]);
            }
          }
        }

        /* Castling */
        if (us == COLOR_WHITE && r == 7 && c == 4 && !game_is_square_attacked(g, 7, 4, them)) {
          if (g->castle_wk &&
              g->board[7][7].type == PIECE_ROOK && g->board[7][7].color == COLOR_WHITE &&
              g->board[7][5].type == PIECE_NONE && g->board[7][6].type == PIECE_NONE &&
              !game_is_square_attacked(g, 7, 5, them) &&
              !game_is_square_attacked(g, 7, 6, them)) {
            add_move(moves, &count, 7, 4, 7, 6, PIECE_NONE, true, false, none_piece);
          }
          if (g->castle_wq &&
              g->board[7][0].type == PIECE_ROOK && g->board[7][0].color == COLOR_WHITE &&
              g->board[7][1].type == PIECE_NONE && g->board[7][2].type == PIECE_NONE &&
              g->board[7][3].type == PIECE_NONE &&
              !game_is_square_attacked(g, 7, 3, them) &&
              !game_is_square_attacked(g, 7, 2, them)) {
            add_move(moves, &count, 7, 4, 7, 2, PIECE_NONE, true, false, none_piece);
          }
        } else if (us == COLOR_BLACK && r == 0 && c == 4 && !game_is_square_attacked(g, 0, 4, them)) {
          if (g->castle_bk &&
              g->board[0][7].type == PIECE_ROOK && g->board[0][7].color == COLOR_BLACK &&
              g->board[0][5].type == PIECE_NONE && g->board[0][6].type == PIECE_NONE &&
              !game_is_square_attacked(g, 0, 5, them) &&
              !game_is_square_attacked(g, 0, 6, them)) {
            add_move(moves, &count, 0, 4, 0, 6, PIECE_NONE, true, false, none_piece);
          }
          if (g->castle_bq &&
              g->board[0][0].type == PIECE_ROOK && g->board[0][0].color == COLOR_BLACK &&
              g->board[0][1].type == PIECE_NONE && g->board[0][2].type == PIECE_NONE &&
              g->board[0][3].type == PIECE_NONE &&
              !game_is_square_attacked(g, 0, 3, them) &&
              !game_is_square_attacked(g, 0, 2, them)) {
            add_move(moves, &count, 0, 4, 0, 2, PIECE_NONE, true, false, none_piece);
          }
        }
      } else {
        /* Sliding pieces: Bishop, Rook, Queen */
        static const int dirs_r[8] = { -1, 1,  0, 0, -1, -1,  1, 1 };
        static const int dirs_c[8] = {  0, 0, -1, 1, -1,  1, -1, 1 };
        int d_start = (p.type == PIECE_BISHOP) ? 4 : 0;
        int d_end   = (p.type == PIECE_ROOK)   ? 4 : 8;

        for (int d = d_start; d < d_end; d++) {
          int nr = r + dirs_r[d];
          int nc = c + dirs_c[d];
          while (in_bounds(nr, nc)) {
            Piece target = g->board[nr][nc];
            if (target.type == PIECE_NONE) {
              add_move(moves, &count, r, c, nr, nc, PIECE_NONE, false, false, none_piece);
            } else {
              if (target.color == them) {
                add_move(moves, &count, r, c, nr, nc, PIECE_NONE, false, false, target);
              }
              break;
            }
            nr += dirs_r[d];
            nc += dirs_c[d];
          }
        }
      }
    }
  }

  return count;
}

int game_generate_legal_moves(const GameState *g, ChessMove *moves) {
  ChessMove pseudo[MAX_MOVES];
  int pseudo_count = game_generate_pseudo_moves(g, pseudo);

  GameState sim;
  memcpy(sim.board, g->board, sizeof(g->board));
  sim.turn = g->turn;
  sim.castle_wk = g->castle_wk;
  sim.castle_wq = g->castle_wq;
  sim.castle_bk = g->castle_bk;
  sim.castle_bq = g->castle_bq;
  sim.ep_r = g->ep_r;
  sim.ep_c = g->ep_c;
  sim.halfmove_clock = g->halfmove_clock;
  sim.fullmove_number = g->fullmove_number;

  int legal_count = 0;
  PieceColor mover = g->turn;

  for (int i = 0; i < pseudo_count; i++) {
    LightUndo u;
    save_light_undo(&sim, &u);
    apply_move_raw(&sim, pseudo[i]);
    if (!game_is_in_check(&sim, mover)) {
      moves[legal_count] = pseudo[i];
      /* Deduct attacker value slightly in MVV-LVA for captures */
      if (moves[legal_count].captured.type != PIECE_NONE) {
        PieceType attacker = g->board[pseudo[i].from_r][pseudo[i].from_c].type;
        moves[legal_count].score_guess -= PIECE_VALUES[attacker] / 10;
      }
      legal_count++;
    }
    restore_light_undo(&sim, &u);
  }

  return legal_count;
}

static bool is_insufficient_material(const GameState *g) {
  int minor_count = 0;
  for (int r = 0; r < BOARD_SIZE; r++) {
    for (int c = 0; c < BOARD_SIZE; c++) {
      PieceType t = g->board[r][c].type;
      if (t == PIECE_NONE || t == PIECE_KING) continue;
      if (t == PIECE_PAWN || t == PIECE_ROOK || t == PIECE_QUEEN) return false;
      minor_count++;
    }
  }
  return (minor_count <= 1);
}

static void format_san_move(const GameState *before, ChessMove m, const GameState *after,
                            char *out_san, size_t size) {
  if (m.is_castling) {
    snprintf(out_san, size, "%s", (m.to_c == 6) ? "O-O" : "O-O-O");
  } else {
    Piece moving = before->board[m.from_r][m.from_c];
    char to_file = (char)('a' + m.to_c);
    char to_rank = (char)('1' + (7 - m.to_r));
    bool is_cap = (m.captured.type != PIECE_NONE || m.is_en_passant);

    if (moving.type == PIECE_PAWN) {
      if (is_cap) {
        char from_file = (char)('a' + m.from_c);
        if (m.promotion != PIECE_NONE) {
          snprintf(out_san, size, "%cx%c%c=%c", from_file, to_file, to_rank, game_piece_letter(m.promotion));
        } else {
          snprintf(out_san, size, "%cx%c%c", from_file, to_file, to_rank);
        }
      } else {
        if (m.promotion != PIECE_NONE) {
          snprintf(out_san, size, "%c%c=%c", to_file, to_rank, game_piece_letter(m.promotion));
        } else {
          snprintf(out_san, size, "%c%c", to_file, to_rank);
        }
      }
    } else {
      char p_let = game_piece_letter(moving.type);
      if (is_cap) {
        snprintf(out_san, size, "%cx%c%c", p_let, to_file, to_rank);
      } else {
        snprintf(out_san, size, "%c%c%c", p_let, to_file, to_rank);
      }
    }
  }

  /* Append + or # if enemy is now in check/checkmate */
  if (after->status == STATUS_CHECKMATE) {
    strncat(out_san, "#", size - strlen(out_san) - 1);
  } else if (game_is_in_check(after, after->turn)) {
    strncat(out_san, "+", size - strlen(out_san) - 1);
  }
}

void game_refresh_legal_moves(GameState *g) {
  g->legal_move_count = game_generate_legal_moves(g, g->legal_moves);

  if (g->legal_move_count == 0) {
    if (game_is_in_check(g, g->turn)) {
      g->status = STATUS_CHECKMATE;
      g->winner = opposite_color(g->turn);
      if (g->winner == g->human_color && g->difficulty != AI_TWO_PLAYER) {
        g->wins_player++;
        game_save_scores(g);
      }
      snprintf(g->status_msg, sizeof(g->status_msg), "CHECKMATE! %s wins!",
               (g->winner == COLOR_WHITE) ? "White" : "Black");
    } else {
      g->status = STATUS_STALEMATE;
      g->winner = COLOR_NONE;
      snprintf(g->status_msg, sizeof(g->status_msg), "STALEMATE! Game drawn.");
    }
  } else if (g->halfmove_clock >= 100) {
    g->status = STATUS_DRAW_50_MOVE;
    g->winner = COLOR_NONE;
    snprintf(g->status_msg, sizeof(g->status_msg), "DRAW by 50-move rule!");
  } else if (is_insufficient_material(g)) {
    g->status = STATUS_DRAW_MATERIAL;
    g->winner = COLOR_NONE;
    snprintf(g->status_msg, sizeof(g->status_msg), "DRAW by insufficient material!");
  } else {
    g->status = STATUS_PLAYING;
  }
}

bool game_make_move(GameState *g, ChessMove m, bool record_history) {
  /* Validate against current legal moves */
  int found_idx = -1;
  for (int i = 0; i < g->legal_move_count; i++) {
    ChessMove lm = g->legal_moves[i];
    if (lm.from_r == m.from_r && lm.from_c == m.from_c &&
        lm.to_r == m.to_r && lm.to_c == m.to_c) {
      if (lm.promotion == m.promotion || (m.promotion == PIECE_NONE && lm.promotion == PIECE_QUEEN)) {
        found_idx = i;
        break;
      }
    }
  }
  if (found_idx == -1) return false;

  ChessMove actual = g->legal_moves[found_idx];
  GameState before_copy = *g;

  BoardUndoState *u = NULL;
  if (record_history) {
    if (g->history_count >= MAX_HISTORY) {
      memmove(&g->history[0], &g->history[1], sizeof(BoardUndoState) * (MAX_HISTORY - 1));
      g->history_count = MAX_HISTORY - 1;
    }
    u = &g->history[g->history_count++];
    memcpy(u->board, g->board, sizeof(g->board));
    u->turn = g->turn;
    u->castle_wk = g->castle_wk;
    u->castle_wq = g->castle_wq;
    u->castle_bk = g->castle_bk;
    u->castle_bq = g->castle_bq;
    u->ep_r = g->ep_r;
    u->ep_c = g->ep_c;
    u->halfmove_clock = g->halfmove_clock;
    u->fullmove_number = g->fullmove_number;
    u->move_made = actual;
    memcpy(u->captured_by_white, g->captured_by_white, sizeof(g->captured_by_white));
    u->cap_white_count = g->cap_white_count;
    memcpy(u->captured_by_black, g->captured_by_black, sizeof(g->captured_by_black));
    u->cap_black_count = g->cap_black_count;
    u->last_move = g->last_move;
    u->has_last_move = g->has_last_move;
  }

  /* Record captured piece into tray */
  if (actual.captured.type != PIECE_NONE) {
    if (g->turn == COLOR_WHITE && g->cap_white_count < 32) {
      g->captured_by_white[g->cap_white_count++] = actual.captured.type;
    } else if (g->turn == COLOR_BLACK && g->cap_black_count < 32) {
      g->captured_by_black[g->cap_black_count++] = actual.captured.type;
    }
  }

  apply_move_raw(g, actual);
  g->last_move = actual;
  g->has_last_move = true;
  g->selected_r = -1;
  g->selected_c = -1;
  g->awaiting_promotion = false;

  game_refresh_legal_moves(g);

  if (u != NULL) {
    format_san_move(&before_copy, actual, g, u->san, sizeof(u->san));
    if (g->status == STATUS_PLAYING) {
      if (game_is_in_check(g, g->turn)) {
        snprintf(g->status_msg, sizeof(g->status_msg), "Played %s — CHECK!", u->san);
      } else {
        snprintf(g->status_msg, sizeof(g->status_msg), "Played %s", u->san);
      }
    }
  }

  return true;
}

static void pop_one_undo(GameState *g) {
  if (g->history_count <= 0) return;
  g->history_count--;
  const BoardUndoState *u = &g->history[g->history_count];
  memcpy(g->board, u->board, sizeof(g->board));
  g->turn = u->turn;
  g->castle_wk = u->castle_wk;
  g->castle_wq = u->castle_wq;
  g->castle_bk = u->castle_bk;
  g->castle_bq = u->castle_bq;
  g->ep_r = u->ep_r;
  g->ep_c = u->ep_c;
  g->halfmove_clock = u->halfmove_clock;
  g->fullmove_number = u->fullmove_number;
  memcpy(g->captured_by_white, u->captured_by_white, sizeof(g->captured_by_white));
  g->cap_white_count = u->cap_white_count;
  memcpy(g->captured_by_black, u->captured_by_black, sizeof(g->captured_by_black));
  g->cap_black_count = u->cap_black_count;
  g->last_move = u->last_move;
  g->has_last_move = u->has_last_move;
}

bool game_undo(GameState *g) {
  if (g->history_count <= 0) {
    snprintf(g->status_msg, sizeof(g->status_msg), "Nothing to undo");
    return false;
  }

  if (g->difficulty != AI_TWO_PLAYER && g->turn == g->human_color && g->history_count >= 2) {
    /* Undo both AI reply and player's last move */
    pop_one_undo(g);
    pop_one_undo(g);
  } else {
    pop_one_undo(g);
  }

  g->selected_r = -1;
  g->selected_c = -1;
  g->awaiting_promotion = false;
  game_refresh_legal_moves(g);
  snprintf(g->status_msg, sizeof(g->status_msg), "Move undone");
  return true;
}

int game_material_advantage_white(const GameState *g) {
  int white_mat = 0;
  int black_mat = 0;
  for (int r = 0; r < BOARD_SIZE; r++) {
    for (int c = 0; c < BOARD_SIZE; c++) {
      Piece p = g->board[r][c];
      if (p.type == PIECE_NONE || p.type == PIECE_KING) continue;
      if (p.color == COLOR_WHITE) white_mat += PIECE_VALUES[p.type] / 100;
      else if (p.color == COLOR_BLACK) black_mat += PIECE_VALUES[p.type] / 100;
    }
  }
  return white_mat - black_mat;
}

/* Evaluate board from the perspective of g->turn (in centipawns) */
int game_evaluate_board(const GameState *g) {
  int white_score = 0;
  int black_score = 0;
  int non_pawn_material = 0;
  int white_bishops = 0;
  int black_bishops = 0;

  for (int r = 0; r < BOARD_SIZE; r++) {
    for (int c = 0; c < BOARD_SIZE; c++) {
      Piece p = g->board[r][c];
      if (p.type != PIECE_NONE && p.type != PIECE_PAWN && p.type != PIECE_KING) {
        non_pawn_material += PIECE_VALUES[p.type];
      }
      if (p.type == PIECE_BISHOP) {
        if (p.color == COLOR_WHITE) white_bishops++;
        else black_bishops++;
      }
    }
  }

  bool is_endgame = (non_pawn_material <= 2600);

  for (int r = 0; r < BOARD_SIZE; r++) {
    for (int c = 0; c < BOARD_SIZE; c++) {
      Piece p = g->board[r][c];
      if (p.type == PIECE_NONE) continue;

      int pst_r = (p.color == COLOR_WHITE) ? r : (7 - r);
      int val = PIECE_VALUES[p.type];

      switch (p.type) {
        case PIECE_PAWN:   val += PST_PAWN[pst_r][c]; break;
        case PIECE_KNIGHT: val += PST_KNIGHT[pst_r][c]; break;
        case PIECE_BISHOP: val += PST_BISHOP[pst_r][c]; break;
        case PIECE_ROOK:   val += PST_ROOK[pst_r][c]; break;
        case PIECE_QUEEN:  val += PST_QUEEN[pst_r][c]; break;
        case PIECE_KING:
          val += is_endgame ? PST_KING_END[pst_r][c] : PST_KING_MID[pst_r][c];
          break;
        default: break;
      }

      if (p.color == COLOR_WHITE) white_score += val;
      else black_score += val;
    }
  }

  /* Bishop pair bonus */
  if (white_bishops >= 2) white_score += 30;
  if (black_bishops >= 2) black_score += 30;

  int diff = white_score - black_score;
  return (g->turn == COLOR_WHITE) ? diff : -diff;
}

static void sort_moves(ChessMove *moves, int count) {
  for (int i = 1; i < count; i++) {
    ChessMove key = moves[i];
    int j = i - 1;
    while (j >= 0 && moves[j].score_guess < key.score_guess) {
      moves[j + 1] = moves[j];
      j--;
    }
    moves[j + 1] = key;
  }
}

/* Quiescence search to resolve capture sequences and avoid horizon blunders */
static int quiescence(GameState *sim, int alpha, int beta, int q_depth) {
  int stand_pat = game_evaluate_board(sim);
  if (stand_pat >= beta) {
    return beta;
  }
  if (alpha < stand_pat) {
    alpha = stand_pat;
  }
  if (q_depth <= 0) {
    return stand_pat;
  }

  ChessMove moves[MAX_MOVES];
  int count = game_generate_legal_moves(sim, moves);
  sort_moves(moves, count);

  for (int i = 0; i < count; i++) {
    /* Only search captures and promotions in quiescence */
    if (moves[i].captured.type == PIECE_NONE && moves[i].promotion == PIECE_NONE) {
      continue;
    }

    LightUndo u;
    save_light_undo(sim, &u);
    apply_move_raw(sim, moves[i]);
    int score = -quiescence(sim, -beta, -alpha, q_depth - 1);
    restore_light_undo(sim, &u);

    if (score >= beta) {
      return beta;
    }
    if (score > alpha) {
      alpha = score;
    }
  }

  return alpha;
}

static int negamax(GameState *sim, int depth, int alpha, int beta, int ply) {
  ChessMove moves[MAX_MOVES];
  int count = game_generate_legal_moves(sim, moves);

  if (count == 0) {
    if (game_is_in_check(sim, sim->turn)) {
      /* Prefer faster checkmates (subtract ply) */
      return -MATE_SCORE + ply;
    }
    return 0; /* Stalemate */
  }

  if (sim->halfmove_clock >= 100) {
    return 0;
  }

  if (depth <= 0) {
    return quiescence(sim, alpha, beta, 4);
  }

  sort_moves(moves, count);

  for (int i = 0; i < count; i++) {
    LightUndo u;
    save_light_undo(sim, &u);
    apply_move_raw(sim, moves[i]);
    int score = -negamax(sim, depth - 1, -beta, -alpha, ply + 1);
    restore_light_undo(sim, &u);

    if (score >= beta) {
      return beta;
    }
    if (score > alpha) {
      alpha = score;
    }
  }

  return alpha;
}

ChessMove game_ai_best_move(const GameState *g, int depth) {
  ChessMove best = { 0 };
  ChessMove moves[MAX_MOVES];
  int count = game_generate_legal_moves(g, moves);
  if (count == 0) return best;

  sort_moves(moves, count);
  best = moves[0];

  GameState sim;
  memcpy(sim.board, g->board, sizeof(g->board));
  sim.turn = g->turn;
  sim.castle_wk = g->castle_wk;
  sim.castle_wq = g->castle_wq;
  sim.castle_bk = g->castle_bk;
  sim.castle_bq = g->castle_bq;
  sim.ep_r = g->ep_r;
  sim.ep_c = g->ep_c;
  sim.halfmove_clock = g->halfmove_clock;
  sim.fullmove_number = g->fullmove_number;

  int best_score = -INF_SCORE;
  int alpha = -INF_SCORE;
  int beta = INF_SCORE;

  for (int i = 0; i < count; i++) {
    LightUndo u;
    save_light_undo(&sim, &u);
    apply_move_raw(&sim, moves[i]);
    int root_alpha = (g->difficulty == AI_EASY) ? -INF_SCORE : alpha;
    int score = -negamax(&sim, depth - 1, -beta, -root_alpha, 1);
    restore_light_undo(&sim, &u);

    /* Add small positional noise in Easy mode for non-forcing moves */
    if (g->difficulty == AI_EASY && abs(score) < MATE_SCORE - 100) {
      score += (rand() % 35) - 17;
    }

    if (score > best_score) {
      best_score = score;
      best = moves[i];
    }
    if (score > alpha) {
      alpha = score;
    }
  }

  return best;
}

bool game_ai_step(GameState *g) {
  if (g->status != STATUS_PLAYING) return false;
  if (g->difficulty == AI_TWO_PLAYER) return false;
  if (g->turn == g->human_color) return false;

  int depth = 3;
  if (g->difficulty == AI_EASY) depth = 2;
  else if (g->difficulty == AI_MEDIUM) depth = 3;
  else if (g->difficulty == AI_HARD) depth = 4;

  ChessMove best = game_ai_best_move(g, depth);
  return game_make_move(g, best, true);
}

bool game_get_hint(const GameState *g, char *out_buf, size_t buf_size) {
  if (g->status != STATUS_PLAYING || g->legal_move_count == 0) {
    snprintf(out_buf, buf_size, "No legal moves available");
    return false;
  }

  ChessMove best = game_ai_best_move(g, 3);
  Piece p = g->board[best.from_r][best.from_c];
  snprintf(out_buf, buf_size, "Hint: Move %s (%c%d -> %c%d)",
           game_piece_symbol(p),
           (char)('a' + best.from_c), 8 - best.from_r,
           (char)('a' + best.to_c),   8 - best.to_r);
  return true;
}

void game_clear_board(GameState *g) {
  memset(g->board, 0, sizeof(g->board));
  g->turn = COLOR_WHITE;
  g->castle_wk = false;
  g->castle_wq = false;
  g->castle_bk = false;
  g->castle_bq = false;
  g->ep_r = -1;
  g->ep_c = -1;
  g->halfmove_clock = 0;
  g->fullmove_number = 1;
  g->status = STATUS_PLAYING;
  g->winner = COLOR_NONE;
  g->selected_r = -1;
  g->selected_c = -1;
  g->awaiting_promotion = false;
  g->history_count = 0;
  g->cap_white_count = 0;
  g->cap_black_count = 0;
  g->has_last_move = false;
}

void game_reset_board(GameState *g) {
  game_clear_board(g);

  static const PieceType back_rank[8] = {
    PIECE_ROOK, PIECE_KNIGHT, PIECE_BISHOP, PIECE_QUEEN,
    PIECE_KING, PIECE_BISHOP, PIECE_KNIGHT, PIECE_ROOK
  };

  for (int c = 0; c < BOARD_SIZE; c++) {
    g->board[0][c] = (Piece){ back_rank[c], COLOR_BLACK };
    g->board[1][c] = (Piece){ PIECE_PAWN,   COLOR_BLACK };
    g->board[6][c] = (Piece){ PIECE_PAWN,   COLOR_WHITE };
    g->board[7][c] = (Piece){ back_rank[c], COLOR_WHITE };
  }

  g->castle_wk = true;
  g->castle_wq = true;
  g->castle_bk = true;
  g->castle_bq = true;

  g->cursor_r = 6;
  g->cursor_c = 4;

  game_refresh_legal_moves(g);
  snprintf(g->status_msg, sizeof(g->status_msg), "New game started! White to move.");
}

void game_init(GameState *g, AIDifficulty diff) {
  memset(g, 0, sizeof(GameState));
  g->difficulty = diff;
  g->human_color = COLOR_WHITE;
  g->board_flipped = false;

  g->wins_player = arcade_score_get("chess", "wins");
  g->games_played = arcade_score_get("chess", "games_played") + 1;
  game_save_scores(g);

  game_reset_board(g);
}

void game_save_scores(const GameState *g) {
  arcade_score_set("chess", "wins", g->wins_player);
  arcade_score_set("chess", "games_played", g->games_played);
}

void game_move_cursor(GameState *g, int dr, int dc) {
  if (g->board_flipped) {
    dr = -dr;
    dc = -dc;
  }
  int nr = g->cursor_r + dr;
  int nc = g->cursor_c + dc;
  if (nr >= 0 && nr < BOARD_SIZE) g->cursor_r = nr;
  if (nc >= 0 && nc < BOARD_SIZE) g->cursor_c = nc;
}

void game_cancel_selection(GameState *g) {
  g->selected_r = -1;
  g->selected_c = -1;
  g->awaiting_promotion = false;
  snprintf(g->status_msg, sizeof(g->status_msg), "Selection cleared");
}

void game_complete_promotion(GameState *g, PieceType promo_type) {
  if (!g->awaiting_promotion) return;
  if (promo_type != PIECE_QUEEN && promo_type != PIECE_ROOK &&
      promo_type != PIECE_BISHOP && promo_type != PIECE_KNIGHT) {
    promo_type = PIECE_QUEEN;
  }

  ChessMove m = { 0 };
  m.from_r = g->promo_from_r;
  m.from_c = g->promo_from_c;
  m.to_r = g->promo_to_r;
  m.to_c = g->promo_to_c;
  m.promotion = promo_type;

  g->awaiting_promotion = false;
  game_make_move(g, m, true);
}

void game_select_square(GameState *g, int r, int c) {
  if (!in_bounds(r, c) || g->status != STATUS_PLAYING) return;
  if (g->awaiting_promotion) return;

  g->cursor_r = r;
  g->cursor_c = c;

  /* If no piece is selected yet, select own piece */
  if (g->selected_r == -1) {
    Piece p = g->board[r][c];
    if (p.color != g->turn) {
      snprintf(g->status_msg, sizeof(g->status_msg), "Select a %s piece to move",
               (g->turn == COLOR_WHITE) ? "White" : "Black");
      return;
    }

    /* Check if this piece has at least one legal move */
    int piece_moves = 0;
    for (int i = 0; i < g->legal_move_count; i++) {
      if (g->legal_moves[i].from_r == r && g->legal_moves[i].from_c == c) {
        piece_moves++;
      }
    }
    if (piece_moves == 0) {
      snprintf(g->status_msg, sizeof(g->status_msg), "That piece has no legal moves");
      return;
    }

    g->selected_r = r;
    g->selected_c = c;
    snprintf(g->status_msg, sizeof(g->status_msg), "Selected %s at %c%d (%d legal move%s)",
             game_piece_symbol(p), (char)('a' + c), 8 - r,
             piece_moves, (piece_moves == 1) ? "" : "s");
    return;
  }

  /* Clicking the already selected square deselects it */
  if (g->selected_r == r && g->selected_c == c) {
    game_cancel_selection(g);
    return;
  }

  /* Clicking another friendly piece switches selection to it */
  if (g->board[r][c].color == g->turn) {
    g->selected_r = -1;
    g->selected_c = -1;
    game_select_square(g, r, c);
    return;
  }

  /* Otherwise check if (selected -> r, c) is a legal move */
  for (int i = 0; i < g->legal_move_count; i++) {
    ChessMove lm = g->legal_moves[i];
    if (lm.from_r == g->selected_r && lm.from_c == g->selected_c &&
        lm.to_r == r && lm.to_c == c) {
      if (lm.promotion != PIECE_NONE) {
        /* Prompt user for promotion piece */
        g->awaiting_promotion = true;
        g->promo_from_r = lm.from_r;
        g->promo_from_c = lm.from_c;
        g->promo_to_r = lm.to_r;
        g->promo_to_c = lm.to_c;
        snprintf(g->status_msg, sizeof(g->status_msg),
                 "PROMOTE PAWN: Press [1]Queen [2]Rook [3]Bishop [4]Knight");
        return;
      }
      game_make_move(g, lm, true);
      return;
    }
  }

  snprintf(g->status_msg, sizeof(g->status_msg), "Illegal move to %c%d", (char)('a' + c), 8 - r);
}

void game_cycle_difficulty(GameState *g) {
  g->difficulty = (AIDifficulty)((g->difficulty + 1) % 4);
  snprintf(g->status_msg, sizeof(g->status_msg), "Mode switched to: %s",
           game_difficulty_name(g->difficulty));
}
