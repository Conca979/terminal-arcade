#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../../include/scores.h"

#define SAVE_MAGIC 0x4B4C4257 // "WBLK"
#define SAVE_VERSION 1

static const Shape SHAPES[] = {
  // 1x1 dot
  {1, 1, 1, {{1}}, 4}, // Yellow

  // 1x2, 2x1
  {2, 1, 2, {{1, 1}}, 1}, // Cyan
  {1, 2, 2, {{1}, {1}}, 1}, // Cyan

  // 1x3, 3x1
  {3, 1, 3, {{1, 1, 1}}, 2}, // Blue
  {1, 3, 3, {{1}, {1}, {1}}, 2}, // Blue

  // 1x4, 4x1
  {4, 1, 4, {{1, 1, 1, 1}}, 1}, // Cyan
  {1, 4, 4, {{1}, {1}, {1}, {1}}, 1}, // Cyan

  // 1x5, 5x1
  {5, 1, 5, {{1, 1, 1, 1, 1}}, 1}, // Cyan
  {1, 5, 5, {{1}, {1}, {1}, {1}, {1}}, 1}, // Cyan

  // 2x2 Square
  {2, 2, 4, {{1, 1}, {1, 1}}, 4}, // Yellow

  // 3x3 Square
  {3, 3, 9, {{1, 1, 1}, {1, 1, 1}, {1, 1, 1}}, 3}, // Orange

  // Small L-corners (2x2)
  {2, 2, 3, {{1, 0}, {1, 1}}, 7}, // Red
  {2, 2, 3, {{0, 1}, {1, 1}}, 7}, // Red
  {2, 2, 3, {{1, 1}, {1, 0}}, 7}, // Red
  {2, 2, 3, {{1, 1}, {0, 1}}, 7}, // Red

  // Big L-shapes (3x3)
  {3, 3, 5, {{1, 0, 0}, {1, 0, 0}, {1, 1, 1}}, 3}, // Orange
  {3, 3, 5, {{0, 0, 1}, {0, 0, 1}, {1, 1, 1}}, 3}, // Orange
  {3, 3, 5, {{1, 1, 1}, {1, 0, 0}, {1, 0, 0}}, 3}, // Orange
  {3, 3, 5, {{1, 1, 1}, {0, 0, 1}, {0, 0, 1}}, 3}, // Orange

  // T-shapes
  {3, 2, 4, {{1, 1, 1}, {0, 1, 0}}, 6}, // Purple
  {3, 2, 4, {{0, 1, 0}, {1, 1, 1}}, 6}, // Purple
  {2, 3, 4, {{1, 0}, {1, 1}, {1, 0}}, 6}, // Purple
  {2, 3, 4, {{0, 1}, {1, 1}, {0, 1}}, 6}, // Purple

  // 2x3 and 3x2 Blocks
  {3, 2, 6, {{1, 1, 1}, {1, 1, 1}}, 5}, // Green
  {2, 3, 6, {{1, 1}, {1, 1}, {1, 1}}, 5}  // Green
};

static const int SHAPE_COUNT = sizeof(SHAPES) / sizeof(SHAPES[0]);

static int load_high_score(void) {
  return arcade_score_get("block", "high_score");
}

static void save_high_score(int score) {
  arcade_score_set("block", "high_score", score);
}

void game_save(const GameState *game) {
  FILE *f = fopen(SAVE_FILE, "wb");
  if (!f) return;

  uint32_t magic = SAVE_MAGIC;
  uint32_t ver = SAVE_VERSION;
  fwrite(&magic, sizeof(uint32_t), 1, f);
  fwrite(&ver, sizeof(uint32_t), 1, f);

  fwrite(&game->score, sizeof(int), 1, f);
  fwrite(&game->lines_cleared, sizeof(int), 1, f);
  fwrite(&game->streak, sizeof(int), 1, f);
  fwrite(&game->blocks_placed, sizeof(int), 1, f);

  fwrite(game->grid, sizeof(uint8_t), GRID_SIZE * GRID_SIZE, f);
  fwrite(game->tray, sizeof(TrayPiece), MAX_PIECES_TRAY, f);

  fclose(f);
}

bool game_load(GameState *game) {
  FILE *f = fopen(SAVE_FILE, "rb");
  if (!f) return false;

  uint32_t magic = 0;
  uint32_t ver = 0;
  if (fread(&magic, sizeof(uint32_t), 1, f) != 1 || magic != SAVE_MAGIC) {
    fclose(f);
    return false;
  }
  if (fread(&ver, sizeof(uint32_t), 1, f) != 1 || ver != SAVE_VERSION) {
    fclose(f);
    return false;
  }

  if (fread(&game->score, sizeof(int), 1, f) != 1 ||
      fread(&game->lines_cleared, sizeof(int), 1, f) != 1 ||
      fread(&game->streak, sizeof(int), 1, f) != 1 ||
      fread(&game->blocks_placed, sizeof(int), 1, f) != 1 ||
      fread(game->grid, sizeof(uint8_t), GRID_SIZE * GRID_SIZE, f) != (GRID_SIZE * GRID_SIZE) ||
      fread(game->tray, sizeof(TrayPiece), MAX_PIECES_TRAY, f) != MAX_PIECES_TRAY) {
    fclose(f);
    return false;
  }

  fclose(f);
  game->cursor_r = 4;
  game->cursor_c = 4;
  game->selected_piece = -1;
  game->game_over = false;
  game->exit_requested = false;
  game->resumed_game = true;
  game->combo_timer = 0;
  game->combo_banner[0] = '\0';

  // If all tray pieces were somehow empty, deal fresh pieces
  bool any_avail = false;
  for (int i = 0; i < MAX_PIECES_TRAY; i++) {
    if (game->tray[i].available) any_avail = true;
  }
  if (!any_avail) {
    game_deal_pieces(game);
  }

  return true;
}

void game_delete_save(void) {
  remove(SAVE_FILE);
}

void game_deal_pieces(GameState *game) {
  for (int i = 0; i < MAX_PIECES_TRAY; i++) {
    int idx = rand() % SHAPE_COUNT;
    game->tray[i].shape = SHAPES[idx];
    game->tray[i].shape.color_id = SHAPES[idx].color_id;
    game->tray[i].available = true;
  }

  // Auto-select first available piece for convenient keyboard play
  game->selected_piece = 0;
}

bool game_can_place(const GameState *game, int tray_idx, int r, int c) {
  if (tray_idx < 0 || tray_idx >= MAX_PIECES_TRAY) return false;
  if (!game->tray[tray_idx].available) return false;

  const Shape *s = &game->tray[tray_idx].shape;
  for (int dr = 0; dr < s->h; dr++) {
    for (int dc = 0; dc < s->w; dc++) {
      if (s->mask[dr][dc] == 1) {
        int tr = r + dr;
        int tc = c + dc;
        if (tr < 0 || tr >= GRID_SIZE || tc < 0 || tc >= GRID_SIZE) {
          return false;
        }
        if (game->grid[tr][tc] != 0) {
          return false;
        }
      }
    }
  }

  return true;
}

bool game_has_any_valid_move(const GameState *game) {
  for (int i = 0; i < MAX_PIECES_TRAY; i++) {
    if (!game->tray[i].available) continue;
    for (int r = 0; r < GRID_SIZE; r++) {
      for (int c = 0; c < GRID_SIZE; c++) {
        if (game_can_place(game, i, r, c)) {
          return true;
        }
      }
    }
  }
  return false;
}

bool game_place_piece(GameState *game, int tray_idx, int r, int c) {
  if (!game_can_place(game, tray_idx, r, c)) {
    return false;
  }

  const Shape *s = &game->tray[tray_idx].shape;
  for (int dr = 0; dr < s->h; dr++) {
    for (int dc = 0; dc < s->w; dc++) {
      if (s->mask[dr][dc] == 1) {
        game->grid[r + dr][c + dc] = (uint8_t)s->color_id;
      }
    }
  }

  game->tray[tray_idx].available = false;
  game->score += s->block_count * 10;
  game->blocks_placed += s->block_count;

  // Check full rows and full columns
  bool full_rows[GRID_SIZE] = {false};
  bool full_cols[GRID_SIZE] = {false};
  int rows_cleared = 0;
  int cols_cleared = 0;

  for (int row = 0; row < GRID_SIZE; row++) {
    bool full = true;
    for (int col = 0; col < GRID_SIZE; col++) {
      if (game->grid[row][col] == 0) {
        full = false;
        break;
      }
    }
    if (full) {
      full_rows[row] = true;
      rows_cleared++;
    }
  }

  for (int col = 0; col < GRID_SIZE; col++) {
    bool full = true;
    for (int row = 0; row < GRID_SIZE; row++) {
      if (game->grid[row][col] == 0) {
        full = false;
        break;
      }
    }
    if (full) {
      full_cols[col] = true;
      cols_cleared++;
    }
  }

  int total_lines = rows_cleared + cols_cleared;
  if (total_lines > 0) {
    // Clear rows
    for (int row = 0; row < GRID_SIZE; row++) {
      if (full_rows[row]) {
        for (int col = 0; col < GRID_SIZE; col++) {
          game->grid[row][col] = 0;
        }
      }
    }

    // Clear cols
    for (int col = 0; col < GRID_SIZE; col++) {
      if (full_cols[col]) {
        for (int row = 0; row < GRID_SIZE; row++) {
          game->grid[row][col] = 0;
        }
      }
    }

    // Scoring & Combos
    int clear_bonus = 0;
    if (total_lines == 1) clear_bonus = 100;
    else if (total_lines == 2) clear_bonus = 300;
    else if (total_lines == 3) clear_bonus = 600;
    else clear_bonus = 1000 + (total_lines - 4) * 300;

    game->streak++;
    int streak_bonus = game->streak * 50;
    game->score += clear_bonus + streak_bonus;
    game->lines_cleared += total_lines;

    if (total_lines == 1) {
      snprintf(game->combo_banner, sizeof(game->combo_banner), "LINE CLEAR! +%d", clear_bonus);
    } else if (total_lines == 2) {
      snprintf(game->combo_banner, sizeof(game->combo_banner), "DOUBLE CLEAR! +%d", clear_bonus);
    } else if (total_lines == 3) {
      snprintf(game->combo_banner, sizeof(game->combo_banner), "TRIPLE COMBO! +%d", clear_bonus);
    } else {
      snprintf(game->combo_banner, sizeof(game->combo_banner), "MEGA %d-LINE CLEAR! +%d", total_lines, clear_bonus);
    }
    game->combo_timer = 40;
  } else {
    game->streak = 0;
  }

  // Check if all tray pieces used
  bool any_left = false;
  for (int i = 0; i < MAX_PIECES_TRAY; i++) {
    if (game->tray[i].available) {
      any_left = true;
      break;
    }
  }

  if (!any_left) {
    game_deal_pieces(game);
  } else {
    // Auto-advance selection to next available piece
    int next_sel = -1;
    for (int i = 0; i < MAX_PIECES_TRAY; i++) {
      if (game->tray[i].available) {
        next_sel = i;
        break;
      }
    }
    game->selected_piece = next_sel;
  }

  // Check game over
  if (!game_has_any_valid_move(game)) {
    game->game_over = true;
    game_delete_save();
    if (game->score > game->high_score) {
      game->high_score = game->score;
      save_high_score(game->high_score);
    }
    return true;
  }

  if (game->score > game->high_score) {
    game->high_score = game->score;
    save_high_score(game->high_score);
  }

  game_save(game);
  return true;
}

void game_reset(GameState *game) {
  memset(game->grid, 0, sizeof(game->grid));
  game->cursor_r = 4;
  game->cursor_c = 4;
  game->selected_piece = 0;
  game->score = 0;
  game->lines_cleared = 0;
  game->streak = 0;
  game->blocks_placed = 0;
  game->game_over = false;
  game->exit_requested = false;
  game->resumed_game = false;
  game->combo_timer = 0;
  game->combo_banner[0] = '\0';
  game->mouse_term_x = 0;
  game->mouse_term_y = 0;

  game_delete_save();
  game_deal_pieces(game);
  game_save(game);
}

void game_init(GameState *game) {
  memset(game, 0, sizeof(GameState));
  game->high_score = load_high_score();

  if (!game_load(game)) {
    game_reset(game);
  }
}

void game_handle_action(GameState *game, GameAction action, int param1, int param2, unsigned long long current_time) {
  (void)current_time;

  if (action == ACT_QUIT) {
    game->exit_requested = true;
    return;
  }

  if (action == ACT_RESTART) {
    game_reset(game);
    return;
  }

  if (game->game_over) {
    return;
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

    case ACT_SELECT_1:
      if (game->tray[0].available) game->selected_piece = 0;
      break;

    case ACT_SELECT_2:
      if (game->tray[1].available) game->selected_piece = 1;
      break;

    case ACT_SELECT_3:
      if (game->tray[2].available) game->selected_piece = 2;
      break;

    case ACT_DESELECT:
      game->selected_piece = -1;
      break;

    case ACT_PLACE:
      if (game->selected_piece >= 0) {
        game_place_piece(game, game->selected_piece, game->cursor_r, game->cursor_c);
      }
      break;

    case ACT_MOUSE_MOVE: {
      game->mouse_term_x = param1;
      game->mouse_term_y = param2;

      // In render.c: Grid top-left is row 5, col 2.
      // Interior cells start at row 6, col 3. Each cell is 3 chars wide.
      int grid_x = param1 - 3;
      int grid_y = param2 - 6;

      if (grid_x >= 0 && grid_x < GRID_SIZE * CELL_WIDTH && grid_y >= 0 && grid_y < GRID_SIZE) {
        game->cursor_c = grid_x / CELL_WIDTH;
        game->cursor_r = grid_y;
      }
      break;
    }

    case ACT_MOUSE_CLICK: {
      int mx = param1;
      int my = param2;

      // 1. Check if clicking on one of the 3 Piece Trays at bottom (rows 18..24)
      // Tray 0: cols 2..17 | Tray 1: cols 21..36 | Tray 2: cols 40..55
      if (my >= 18 && my <= 24) {
        if (mx >= 2 && mx <= 17) {
          if (game->tray[0].available) game->selected_piece = 0;
        } else if (mx >= 21 && mx <= 36) {
          if (game->tray[1].available) game->selected_piece = 1;
        } else if (mx >= 40 && mx <= 55) {
          if (game->tray[2].available) game->selected_piece = 2;
        }
      }
      // 2. Check if clicking on the 10x10 Grid to place
      else {
        int grid_x = mx - 3;
        int grid_y = my - 6;
        if (grid_x >= 0 && grid_x < GRID_SIZE * CELL_WIDTH && grid_y >= 0 && grid_y < GRID_SIZE) {
          int c = grid_x / CELL_WIDTH;
          int r = grid_y;
          game->cursor_r = r;
          game->cursor_c = c;
          if (game->selected_piece >= 0) {
            game_place_piece(game, game->selected_piece, r, c);
          }
        }
      }
      break;
    }

    default:
      break;
  }
}

void game_update(GameState *game, unsigned long long current_time) {
  (void)current_time;
  if (game->combo_timer > 0) {
    game->combo_timer--;
  }
}
