#include "game.h"
#include "scores.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

static void push_undo(GameState *g) {
  if (g->undo_count >= MAX_UNDO) {
    memmove(&g->undo_stack[0], &g->undo_stack[1], sizeof(JigsawUndoState) * (MAX_UNDO - 1));
    g->undo_count = MAX_UNDO - 1;
  }
  JigsawUndoState *u = &g->undo_stack[g->undo_count++];
  memcpy(u->board, g->board, sizeof(g->board));
  memcpy(u->tray_present, g->tray_present, sizeof(g->tray_present));
  u->holding_piece = g->holding_piece;
  u->moves_count = g->moves_count;
  u->snapped_count = g->snapped_count;
}

static void update_snapped_count(GameState *g) {
  int count = 0;
  for (int r = 0; r < GRID_SIZE; r++) {
    for (int c = 0; c < GRID_SIZE; c++) {
      int expected = r * GRID_SIZE + c;
      if (g->board[r][c] == expected) {
        count++;
      }
    }
  }
  g->snapped_count = count;

  if (count == TOTAL_PIECES && !g->is_solved) {
    g->is_solved = true;
    g->puzzles_solved++;
    if (g->moves_count < g->best_moves || g->best_moves == 0) {
      g->best_moves = g->moves_count;
    }
    int time_sec = (int)g->elapsed_seconds;
    if (time_sec < g->best_time || g->best_time == 0) {
      g->best_time = time_sec;
    }
    game_save_scores(g);
    snprintf(g->status_msg, sizeof(g->status_msg),
             "★ PUZZLE SOLVED in %d moves! [T] Next Theme  [R] Replay", g->moves_count);
  }
}

void game_reset(GameState *g, int theme_idx) {
  if (theme_idx < 0 || theme_idx >= THEME_COUNT) theme_idx = 0;
  g->current_theme = theme_idx;

  for (int r = 0; r < GRID_SIZE; r++) {
    for (int c = 0; c < GRID_SIZE; c++) {
      g->board[r][c] = -1;
    }
  }

  for (int i = 0; i < TOTAL_PIECES; i++) {
    g->tray_order[i] = i;
    g->tray_present[i] = true;
  }

  /* Fisher-Yates shuffle for tray order */
  for (int i = TOTAL_PIECES - 1; i > 0; i--) {
    int j = rand() % (i + 1);
    int tmp = g->tray_order[i];
    g->tray_order[i] = g->tray_order[j];
    g->tray_order[j] = tmp;
  }

  g->holding_piece = -1;
  g->cursor_board_r = 0;
  g->cursor_board_c = 0;
  g->cursor_tray_idx = 0;
  g->focus = FOCUS_TRAY;
  g->show_preview = false;

  g->hint_piece_id = -1;
  g->hint_target_r = -1;
  g->hint_target_c = -1;
  g->hint_timer_ticks = 0;

  g->moves_count = 0;
  g->snapped_count = 0;
  g->is_solved = false;
  g->elapsed_seconds = 0.0;
  g->undo_count = 0;

  jigsaw_get_theme(theme_idx);
  snprintf(g->status_msg, sizeof(g->status_msg),
           "Select a piece from the tray to begin! [Mouse or Space]");
}

void game_init(GameState *g, int theme_idx) {
  memset(g, 0, sizeof(GameState));
  srand((unsigned int)time(NULL));

  g->puzzles_solved = arcade_score_get("jigsaw", "puzzles_solved");
  g->best_moves = arcade_score_get("jigsaw", "best_moves");
  g->best_time = arcade_score_get("jigsaw", "best_time");

  game_reset(g, theme_idx);
}

void game_next_theme(GameState *g) {
  int next = (g->current_theme + 1) % THEME_COUNT;
  game_reset(g, next);
}

bool game_pick_from_tray(GameState *g, int piece_id) {
  if (piece_id < 0 || piece_id >= TOTAL_PIECES) return false;
  if (!g->tray_present[piece_id]) return false;

  push_undo(g);

  /* If player is already holding a piece, return the old one back to tray */
  if (g->holding_piece != -1) {
    g->tray_present[g->holding_piece] = true;
  }

  g->holding_piece = piece_id;
  g->tray_present[piece_id] = false;
  g->focus = FOCUS_BOARD;

  snprintf(g->status_msg, sizeof(g->status_msg),
           "Holding Piece #%02d. Move to board slot and press Space / Click.",
           piece_id + 1);
  return true;
}

bool game_place_on_board(GameState *g, int r, int c) {
  if (r < 0 || r >= GRID_SIZE || c < 0 || c >= GRID_SIZE) return false;

  /* Case 1: Holding a piece -> place or swap */
  if (g->holding_piece != -1) {
    push_undo(g);
    int placing = g->holding_piece;
    int existing = g->board[r][c];

    g->board[r][c] = placing;
    g->holding_piece = existing; /* Either -1 if slot was empty, or old piece */
    g->moves_count++;

    int expected = r * GRID_SIZE + c;
    if (placing == expected) {
      snprintf(g->status_msg, sizeof(g->status_msg),
               "SNAP! Piece #%02d locked into %c%d!", placing + 1, 'A' + c, r + 1);
    } else {
      snprintf(g->status_msg, sizeof(g->status_msg),
               "Placed Piece #%02d at %c%d.", placing + 1, 'A' + c, r + 1);
    }

    if (g->holding_piece == -1) {
      g->focus = FOCUS_BOARD;
    }

    update_snapped_count(g);
    return true;
  }

  /* Case 2: Not holding a piece -> pick up from board */
  if (g->board[r][c] != -1) {
    push_undo(g);
    g->holding_piece = g->board[r][c];
    g->board[r][c] = -1;
    snprintf(g->status_msg, sizeof(g->status_msg),
             "Picked up Piece #%02d from %c%d.", g->holding_piece + 1, 'A' + c, r + 1);
    update_snapped_count(g);
    return true;
  }

  return false;
}

bool game_return_holding_to_tray(GameState *g) {
  if (g->holding_piece == -1) return false;

  push_undo(g);
  g->tray_present[g->holding_piece] = true;
  snprintf(g->status_msg, sizeof(g->status_msg),
           "Returned Piece #%02d to tray.", g->holding_piece + 1);
  g->holding_piece = -1;
  g->focus = FOCUS_TRAY;
  return true;
}

bool game_return_slot_to_tray(GameState *g, int r, int c) {
  if (r < 0 || r >= GRID_SIZE || c < 0 || c >= GRID_SIZE) return false;
  if (g->board[r][c] == -1) return false;

  push_undo(g);
  int p = g->board[r][c];
  g->board[r][c] = -1;
  g->tray_present[p] = true;

  snprintf(g->status_msg, sizeof(g->status_msg),
           "Returned Piece #%02d to tray.", p + 1);
  update_snapped_count(g);
  return true;
}

void game_interact_current_focus(GameState *g) {
  if (g->focus == FOCUS_TRAY) {
    int piece_id = g->tray_order[g->cursor_tray_idx];
    if (g->tray_present[piece_id]) {
      game_pick_from_tray(g, piece_id);
    } else if (g->holding_piece != -1) {
      game_return_holding_to_tray(g);
    }
  } else {
    game_place_on_board(g, g->cursor_board_r, g->cursor_board_c);
  }
}

void game_toggle_focus(GameState *g) {
  g->focus = (g->focus == FOCUS_BOARD) ? FOCUS_TRAY : FOCUS_BOARD;
}

void game_move_cursor(GameState *g, int dr, int dc) {
  if (g->focus == FOCUS_BOARD) {
    int nr = g->cursor_board_r + dr;
    int nc = g->cursor_board_c + dc;
    if (nr >= 0 && nr < GRID_SIZE) g->cursor_board_r = nr;
    if (nc >= 0 && nc < GRID_SIZE) g->cursor_board_c = nc;
  } else {
    int tr = g->cursor_tray_idx / GRID_SIZE + dr;
    int tc = g->cursor_tray_idx % GRID_SIZE + dc;
    if (tr >= 0 && tr < GRID_SIZE && tc >= 0 && tc < GRID_SIZE) {
      g->cursor_tray_idx = tr * GRID_SIZE + tc;
    }
  }
}

void game_request_hint(GameState *g) {
  if (g->is_solved) return;

  if (g->holding_piece != -1) {
    int tr = g->holding_piece / GRID_SIZE;
    int tc = g->holding_piece % GRID_SIZE;
    g->hint_piece_id = g->holding_piece;
    g->hint_target_r = tr;
    g->hint_target_c = tc;
    g->hint_timer_ticks = 180;
    snprintf(g->status_msg, sizeof(g->status_msg),
             "HINT: Held Piece #%02d belongs at slot %c%d!",
             g->holding_piece + 1, 'A' + tc, tr + 1);
    return;
  }

  /* Find next piece that is not snapped */
  for (int slot = 0; slot < TOTAL_PIECES; slot++) {
    int tr = slot / GRID_SIZE;
    int tc = slot % GRID_SIZE;
    if (g->board[tr][tc] != slot) {
      g->hint_piece_id = slot;
      g->hint_target_r = tr;
      g->hint_target_c = tc;
      g->hint_timer_ticks = 180;
      snprintf(g->status_msg, sizeof(g->status_msg),
               "HINT: Piece #%02d belongs at slot %c%d!", slot + 1, 'A' + tc, tr + 1);
      return;
    }
  }
}

void game_toggle_preview(GameState *g) {
  g->show_preview = !g->show_preview;
  snprintf(g->status_msg, sizeof(g->status_msg),
           g->show_preview ? "Artwork preview reference: ON" : "Artwork preview reference: OFF");
}

bool game_undo(GameState *g) {
  if (g->undo_count <= 0) {
    snprintf(g->status_msg, sizeof(g->status_msg), "Nothing to undo.");
    return false;
  }

  JigsawUndoState *u = &g->undo_stack[--g->undo_count];
  memcpy(g->board, u->board, sizeof(g->board));
  memcpy(g->tray_present, u->tray_present, sizeof(g->tray_present));
  g->holding_piece = u->holding_piece;
  g->moves_count = u->moves_count;
  g->snapped_count = u->snapped_count;

  snprintf(g->status_msg, sizeof(g->status_msg), "Undone last action.");
  return true;
}

void game_update(GameState *g, double dt) {
  if (!g->is_solved) {
    g->elapsed_seconds += dt;
  }
  if (g->hint_timer_ticks > 0) {
    g->hint_timer_ticks--;
    if (g->hint_timer_ticks == 0) {
      g->hint_piece_id = -1;
      g->hint_target_r = -1;
      g->hint_target_c = -1;
    }
  }
}

void game_save_scores(const GameState *g) {
  arcade_score_set("jigsaw", "puzzles_solved", g->puzzles_solved);
  arcade_score_set("jigsaw", "best_moves", g->best_moves);
  arcade_score_set("jigsaw", "best_time", g->best_time);
}
