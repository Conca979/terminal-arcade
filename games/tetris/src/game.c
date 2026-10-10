#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Standard SRS piece definitions: 4 blocks per rotation state
static const Point PIECE_BLOCKS[TET_COUNT][4][4] = {
  // TET_NONE
  {{{0, 0}, {0, 0}, {0, 0}, {0, 0}}},

  // TET_I
  {
    {{0, 1}, {1, 1}, {2, 1}, {3, 1}}, // Rot 0
    {{2, 0}, {2, 1}, {2, 2}, {2, 3}}, // Rot 1
    {{0, 2}, {1, 2}, {2, 2}, {3, 2}}, // Rot 2
    {{1, 0}, {1, 1}, {1, 2}, {1, 3}}  // Rot 3
  },

  // TET_J
  {
    {{0, 0}, {0, 1}, {1, 1}, {2, 1}}, // Rot 0
    {{1, 0}, {2, 0}, {1, 1}, {1, 2}}, // Rot 1
    {{0, 1}, {1, 1}, {2, 1}, {2, 2}}, // Rot 2
    {{1, 0}, {1, 1}, {0, 2}, {1, 2}}  // Rot 3
  },

  // TET_L
  {
    {{2, 0}, {0, 1}, {1, 1}, {2, 1}}, // Rot 0
    {{1, 0}, {1, 1}, {1, 2}, {2, 2}}, // Rot 1
    {{0, 1}, {1, 1}, {2, 1}, {0, 2}}, // Rot 2
    {{0, 0}, {1, 0}, {1, 1}, {1, 2}}  // Rot 3
  },

  // TET_O
  {
    {{1, 0}, {2, 0}, {1, 1}, {2, 1}}, // Rot 0
    {{1, 0}, {2, 0}, {1, 1}, {2, 1}}, // Rot 1
    {{1, 0}, {2, 0}, {1, 1}, {2, 1}}, // Rot 2
    {{1, 0}, {2, 0}, {1, 1}, {2, 1}}  // Rot 3
  },

  // TET_S
  {
    {{1, 0}, {2, 0}, {0, 1}, {1, 1}}, // Rot 0
    {{1, 0}, {1, 1}, {2, 1}, {2, 2}}, // Rot 1
    {{1, 1}, {2, 1}, {0, 2}, {1, 2}}, // Rot 2
    {{0, 0}, {0, 1}, {1, 1}, {1, 2}}  // Rot 3
  },

  // TET_T
  {
    {{1, 0}, {0, 1}, {1, 1}, {2, 1}}, // Rot 0
    {{1, 0}, {1, 1}, {2, 1}, {1, 2}}, // Rot 1
    {{0, 1}, {1, 1}, {2, 1}, {1, 2}}, // Rot 2
    {{1, 0}, {0, 1}, {1, 1}, {1, 2}}  // Rot 3
  },

  // TET_Z
  {
    {{0, 0}, {1, 0}, {1, 1}, {2, 1}}, // Rot 0
    {{2, 0}, {1, 1}, {2, 1}, {1, 2}}, // Rot 1
    {{0, 1}, {1, 1}, {1, 2}, {2, 2}}, // Rot 2
    {{1, 0}, {0, 1}, {1, 1}, {0, 2}}  // Rot 3
  }
};

// SRS Wall kick offsets
static const Point KICKS_JLSTZ[8][5] = {
  // 0: 0 -> 1
  {{0, 0}, {-1, 0}, {-1, -1}, {0, 2}, {-1, 2}},
  // 1: 1 -> 0
  {{0, 0}, {1, 0}, {1, 1}, {0, -2}, {1, -2}},
  // 2: 1 -> 2
  {{0, 0}, {1, 0}, {1, 1}, {0, -2}, {1, -2}},
  // 3: 2 -> 1
  {{0, 0}, {-1, 0}, {-1, -1}, {0, 2}, {-1, 2}},
  // 4: 2 -> 3
  {{0, 0}, {1, 0}, {1, -1}, {0, 2}, {1, 2}},
  // 5: 3 -> 2
  {{0, 0}, {-1, 0}, {-1, 1}, {0, -2}, {-1, -2}},
  // 6: 3 -> 0
  {{0, 0}, {-1, 0}, {-1, 1}, {0, -2}, {-1, -2}},
  // 7: 0 -> 3
  {{0, 0}, {1, 0}, {1, -1}, {0, 2}, {1, 2}}
};

static const Point KICKS_I[8][5] = {
  // 0: 0 -> 1
  {{0, 0}, {-2, 0}, {1, 0}, {-2, 1}, {1, -2}},
  // 1: 1 -> 0
  {{0, 0}, {2, 0}, {-1, 0}, {2, -1}, {-1, 2}},
  // 2: 1 -> 2
  {{0, 0}, {-1, 0}, {2, 0}, {-1, -2}, {2, 1}},
  // 3: 2 -> 1
  {{0, 0}, {1, 0}, {-2, 0}, {1, 2}, {-2, -1}},
  // 4: 2 -> 3
  {{0, 0}, {2, 0}, {-1, 0}, {2, -1}, {-1, 2}},
  // 5: 3 -> 2
  {{0, 0}, {-2, 0}, {1, 0}, {-2, 1}, {1, -2}},
  // 6: 3 -> 0
  {{0, 0}, {1, 0}, {-2, 0}, {1, 2}, {-2, -1}},
  // 7: 0 -> 3
  {{0, 0}, {-1, 0}, {2, 0}, {-1, -2}, {2, 1}}
};

static int get_kick_index(int from_rot, int to_rot) {
  if (from_rot == 0 && to_rot == 1) return 0;
  if (from_rot == 1 && to_rot == 0) return 1;
  if (from_rot == 1 && to_rot == 2) return 2;
  if (from_rot == 2 && to_rot == 1) return 3;
  if (from_rot == 2 && to_rot == 3) return 4;
  if (from_rot == 3 && to_rot == 2) return 5;
  if (from_rot == 3 && to_rot == 0) return 6;
  if (from_rot == 0 && to_rot == 3) return 7;
  return -1;
}

static void fill_and_shuffle_bag(GameState *game) {
  for (int i = 0; i < 7; i++) {
    game->bag[i] = (TetrominoType)(i + 1);
  }
  for (int i = 6; i > 0; i--) {
    int j = rand() % (i + 1);
    TetrominoType temp = game->bag[i];
    game->bag[i] = game->bag[j];
    game->bag[j] = temp;
  }
  game->bag_index = 0;
}

static TetrominoType get_next_from_bag(GameState *game) {
  if (game->bag_index >= 7) {
    fill_and_shuffle_bag(game);
  }
  return game->bag[game->bag_index++];
}

static void replenish_queue(GameState *game) {
  while (game->next_queue_count < 14) {
    game->next_queue[game->next_queue_count++] = get_next_from_bag(game);
  }
}

static TetrominoType pop_queue(GameState *game) {
  if (game->next_queue_count == 0) {
    replenish_queue(game);
  }
  TetrominoType first = game->next_queue[0];
  for (int i = 0; i < game->next_queue_count - 1; i++) {
    game->next_queue[i] = game->next_queue[i + 1];
  }
  game->next_queue_count--;
  replenish_queue(game);
  return first;
}

void game_get_piece_blocks(TetrominoType type, int rotation, Point out_blocks[4]) {
  if (type < 1 || type >= TET_COUNT) {
    for (int i = 0; i < 4; i++) {
      out_blocks[i].x = 0;
      out_blocks[i].y = 0;
    }
    return;
  }
  int rot = rotation % 4;
  for (int i = 0; i < 4; i++) {
    out_blocks[i] = PIECE_BLOCKS[type][rot][i];
  }
}

bool game_check_collision(const GameState *game, const ActivePiece *piece) {
  Point blocks[4];
  game_get_piece_blocks(piece->type, piece->rotation, blocks);

  for (int i = 0; i < 4; i++) {
    int wx = piece->x + blocks[i].x;
    int wy = piece->y + blocks[i].y;

    if (wx < 0 || wx >= BOARD_WIDTH) {
      return true;
    }
    if (wy >= TOTAL_HEIGHT) {
      return true;
    }
    if (wy >= 0 && game->board[wy][wx] != TET_NONE) {
      return true;
    }
  }
  return false;
}

int game_get_ghost_y(const GameState *game) {
  ActivePiece ghost = game->current;
  while (!game_check_collision(game, &ghost)) {
    ghost.y++;
  }
  return ghost.y - 1;
}

int game_get_fall_interval_ms(int level) {
  switch (level) {
    case 1:  return 800;
    case 2:  return 715;
    case 3:  return 630;
    case 4:  return 550;
    case 5:  return 470;
    case 6:  return 395;
    case 7:  return 325;
    case 8:  return 260;
    case 9:  return 200;
    case 10: return 150;
    default: {
      int ms = 150 - (level - 10) * 15;
      return ms < 50 ? 50 : ms;
    }
  }
}

static void spawn_piece(GameState *game, unsigned long long current_time) {
  TetrominoType next_type = pop_queue(game);
  game->current.type = next_type;
  game->current.rotation = 0;
  game->current.x = (next_type == TET_O) ? 4 : 3;
  game->current.y = 0;

  game->can_hold = true;
  game->is_locking = false;
  game->lock_moves_count = 0;
  game->last_fall_time = current_time;

  if (game_check_collision(game, &game->current)) {
    game->state = STATE_GAME_OVER;
  }
}

static void lock_piece_to_board(GameState *game, unsigned long long current_time) {
  Point blocks[4];
  game_get_piece_blocks(game->current.type, game->current.rotation, blocks);

  bool topped_out = false;
  for (int i = 0; i < 4; i++) {
    int wx = game->current.x + blocks[i].x;
    int wy = game->current.y + blocks[i].y;
    if (wy < BUFFER_HEIGHT) {
      topped_out = true;
    }
    if (wy >= 0 && wy < TOTAL_HEIGHT && wx >= 0 && wx < BOARD_WIDTH) {
      game->board[wy][wx] = game->current.type;
    }
  }

  if (topped_out) {
    game->state = STATE_GAME_OVER;
    return;
  }

  // Detect complete lines
  game->clearing_row_count = 0;
  for (int y = TOTAL_HEIGHT - 1; y >= BUFFER_HEIGHT; y--) {
    bool full = true;
    for (int x = 0; x < BOARD_WIDTH; x++) {
      if (game->board[y][x] == TET_NONE) {
        full = false;
        break;
      }
    }
    if (full) {
      game->clearing_rows[game->clearing_row_count++] = y;
    }
  }

  if (game->clearing_row_count > 0) {
    game->state = STATE_LINE_CLEAR;
    game->line_clear_start_time = current_time;
  } else {
    spawn_piece(game, current_time);
  }
}

void game_load_high_score(GameState *game) {
  game->high_score = 0;
  FILE *f = fopen(HIGH_SCORE_FILE, "r");
  if (f) {
    if (fscanf(f, "%d", &game->high_score) != 1) {
      game->high_score = 0;
    }
    fclose(f);
  }
}

void game_save_high_score(const GameState *game) {
  FILE *f = fopen(HIGH_SCORE_FILE, "w");
  if (f) {
    fprintf(f, "%d\n", game->high_score);
    fclose(f);
  }
}

void game_reset(GameState *game) {
  memset(game->board, 0, sizeof(game->board));
  game->hold_piece = TET_NONE;
  game->can_hold = true;
  game->next_queue_count = 0;
  game->score = 0;
  game->lines_cleared = 0;
  game->level = 1;
  game->state = STATE_PLAYING;
  game->is_locking = false;
  game->lock_moves_count = 0;
  game->clearing_row_count = 0;

  fill_and_shuffle_bag(game);
  replenish_queue(game);
  spawn_piece(game, 0);
}

void game_init(GameState *game) {
  game_load_high_score(game);
  game_reset(game);
}

static void try_rotate(GameState *game, bool clockwise, unsigned long long current_time) {
  int from_rot = game->current.rotation;
  int to_rot = clockwise ? (from_rot + 1) % 4 : (from_rot + 3) % 4;

  if (game->current.type == TET_O) {
    return;
  }

  int kick_idx = get_kick_index(from_rot, to_rot);
  const Point (*kicks)[5] = (game->current.type == TET_I) ? KICKS_I : KICKS_JLSTZ;

  for (int k = 0; k < 5; k++) {
    ActivePiece test = game->current;
    test.rotation = to_rot;
    if (kick_idx >= 0) {
      test.x += kicks[kick_idx][k].x;
      test.y += kicks[kick_idx][k].y;
    }

    if (!game_check_collision(game, &test)) {
      game->current = test;
      if (game->is_locking && game->lock_moves_count < MAX_LOCK_MOVES) {
        game->lock_start_time = current_time;
        game->lock_moves_count++;
      }
      return;
    }
  }
}

void game_handle_action(GameState *game, GameAction action, unsigned long long current_time) {
  if (action == ACT_QUIT) {
    game->state = STATE_EXIT;
    return;
  }

  if (action == ACT_PAUSE) {
    if (game->state == STATE_PLAYING) {
      game->state = STATE_PAUSED;
    } else if (game->state == STATE_PAUSED) {
      game->state = STATE_PLAYING;
      game->last_fall_time = current_time;
      if (game->is_locking) {
        game->lock_start_time = current_time;
      }
    }
    return;
  }

  if (action == ACT_RESTART) {
    game_reset(game);
    return;
  }

  if (game->state != STATE_PLAYING) {
    return;
  }

  switch (action) {
    case ACT_LEFT: {
      ActivePiece test = game->current;
      test.x--;
      if (!game_check_collision(game, &test)) {
        game->current.x--;
        if (game->is_locking && game->lock_moves_count < MAX_LOCK_MOVES) {
          game->lock_start_time = current_time;
          game->lock_moves_count++;
        }
      }
      break;
    }

    case ACT_RIGHT: {
      ActivePiece test = game->current;
      test.x++;
      if (!game_check_collision(game, &test)) {
        game->current.x++;
        if (game->is_locking && game->lock_moves_count < MAX_LOCK_MOVES) {
          game->lock_start_time = current_time;
          game->lock_moves_count++;
        }
      }
      break;
    }

    case ACT_DOWN: {
      ActivePiece test = game->current;
      test.y++;
      if (!game_check_collision(game, &test)) {
        game->current.y++;
        game->score += 1;
        game->last_fall_time = current_time;
      } else {
        if (!game->is_locking) {
          game->is_locking = true;
          game->lock_start_time = current_time;
        }
      }
      break;
    }

    case ACT_HARD_DROP: {
      int ghost_y = game_get_ghost_y(game);
      int drop_distance = ghost_y - game->current.y;
      game->score += drop_distance * 2;
      game->current.y = ghost_y;
      lock_piece_to_board(game, current_time);
      break;
    }

    case ACT_ROTATE_CW:
      try_rotate(game, true, current_time);
      break;

    case ACT_ROTATE_CCW:
      try_rotate(game, false, current_time);
      break;

    case ACT_HOLD: {
      if (!game->can_hold) {
        break;
      }
      TetrominoType current_type = game->current.type;
      if (game->hold_piece == TET_NONE) {
        game->hold_piece = current_type;
        spawn_piece(game, current_time);
      } else {
        TetrominoType prev_hold = game->hold_piece;
        game->hold_piece = current_type;
        game->current.type = prev_hold;
        game->current.rotation = 0;
        game->current.x = (prev_hold == TET_O) ? 4 : 3;
        game->current.y = 0;
        game->is_locking = false;
        game->lock_moves_count = 0;
        game->last_fall_time = current_time;
        if (game_check_collision(game, &game->current)) {
          game->state = STATE_GAME_OVER;
        }
      }
      game->can_hold = false;
      break;
    }

    default:
      break;
  }
}

void game_update(GameState *game, unsigned long long current_time) {
  if (game->state == STATE_LINE_CLEAR) {
    if (current_time - game->line_clear_start_time >= LINE_CLEAR_DELAY_MS) {
      // Collapse board: copy non-cleared rows downward
      int dest_y = TOTAL_HEIGHT - 1;
      for (int y = TOTAL_HEIGHT - 1; y >= 0; y--) {
        bool is_cleared = false;
        for (int i = 0; i < game->clearing_row_count; i++) {
          if (game->clearing_rows[i] == y) {
            is_cleared = true;
            break;
          }
        }
        if (!is_cleared) {
          if (dest_y != y) {
            for (int x = 0; x < BOARD_WIDTH; x++) {
              game->board[dest_y][x] = game->board[y][x];
            }
          }
          dest_y--;
        }
      }
      while (dest_y >= 0) {
        for (int x = 0; x < BOARD_WIDTH; x++) {
          game->board[dest_y][x] = TET_NONE;
        }
        dest_y--;
      }

      // Add scores based on cleared rows
      int base_scores[5] = {0, 100, 300, 500, 800};
      int count = game->clearing_row_count;
      if (count > 4) count = 4;
      game->score += base_scores[count] * game->level;

      game->lines_cleared += game->clearing_row_count;
      game->level = 1 + (game->lines_cleared / 10);

      if (game->score > game->high_score) {
        game->high_score = game->score;
        game_save_high_score(game);
      }

      game->clearing_row_count = 0;
      spawn_piece(game, current_time);
      game->state = STATE_PLAYING;
    }
    return;
  }

  if (game->state != STATE_PLAYING) {
    return;
  }

  // Check if resting on ground
  ActivePiece test_down = game->current;
  test_down.y++;
  bool on_ground = game_check_collision(game, &test_down);

  if (on_ground) {
    if (!game->is_locking) {
      game->is_locking = true;
      game->lock_start_time = current_time;
    } else if (current_time - game->lock_start_time >= LOCK_DELAY_MS) {
      lock_piece_to_board(game, current_time);
      return;
    }
  } else {
    game->is_locking = false;
  }

  // Gravity fall
  int fall_interval = game_get_fall_interval_ms(game->level);
  if (current_time - game->last_fall_time >= (unsigned long long)fall_interval) {
    game->last_fall_time = current_time;
    if (!on_ground) {
      game->current.y++;
      // Re-check after fall
      test_down = game->current;
      test_down.y++;
      if (game_check_collision(game, &test_down)) {
        game->is_locking = true;
        game->lock_start_time = current_time;
      }
    }
  }
}
