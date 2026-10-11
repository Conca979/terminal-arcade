#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "../../../include/scores.h"

void game_spawn_tile(GameState *game) {
  int empty_coords[GRID_SIZE * GRID_SIZE][2];
  int empty_count = 0;

  for (int r = 0; r < GRID_SIZE; r++) {
    for (int c = 0; c < GRID_SIZE; c++) {
      if (game->board[r][c] == 0) {
        empty_coords[empty_count][0] = r;
        empty_coords[empty_count][1] = c;
        empty_count++;
      }
    }
  }

  if (empty_count == 0) {
    return;
  }

  int pick = rand() % empty_count;
  int r = empty_coords[pick][0];
  int c = empty_coords[pick][1];

  // 90% chance of 2, 10% chance of 4
  game->board[r][c] = (rand() % 10 == 0) ? 4 : 2;
}

void game_load_high_score(GameState *game) {
  game->high_score = arcade_score_get("2048", "high_score");
}

void game_save_high_score(const GameState *game) {
  arcade_score_set("2048", "high_score", game->high_score);
}

static const char *get_save_path(void) {
  FILE *f = fopen("games/2048/Makefile", "r");
  if (f) {
    fclose(f);
    return "games/2048/savegame.dat";
  }
  return "savegame.dat";
}

#define SAVE_MAGIC_2048 0x32303438 // "2048"
#define SAVE_VERSION_2048 1

void game_save(const GameState *game) {
  if (game->state == STATE_GAME_OVER) return;

  FILE *f = fopen(get_save_path(), "wb");
  if (!f) return;

  uint32_t magic = SAVE_MAGIC_2048;
  uint32_t ver = SAVE_VERSION_2048;
  fwrite(&magic, sizeof(uint32_t), 1, f);
  fwrite(&ver, sizeof(uint32_t), 1, f);

  fwrite(game->board, sizeof(int), GRID_SIZE * GRID_SIZE, f);
  fwrite(&game->score, sizeof(int), 1, f);
  fwrite(&game->move_count, sizeof(int), 1, f);
  fwrite(&game->last_score_gain, sizeof(int), 1, f);
  int st = (int)game->state;
  fwrite(&st, sizeof(int), 1, f);
  int won = game->won_acknowledged ? 1 : 0;
  fwrite(&won, sizeof(int), 1, f);

  fwrite(&game->history_count, sizeof(int), 1, f);
  if (game->history_count > 0 && game->history_count <= MAX_UNDO_STATES) {
    fwrite(game->history, sizeof(BoardSnapshot), game->history_count, f);
  }

  fclose(f);
}

bool game_load(GameState *game) {
  FILE *f = fopen(get_save_path(), "rb");
  if (!f) return false;

  uint32_t magic = 0;
  uint32_t ver = 0;
  if (fread(&magic, sizeof(uint32_t), 1, f) != 1 || magic != SAVE_MAGIC_2048) {
    fclose(f);
    return false;
  }
  if (fread(&ver, sizeof(uint32_t), 1, f) != 1 || ver != SAVE_VERSION_2048) {
    fclose(f);
    return false;
  }

  if (fread(game->board, sizeof(int), GRID_SIZE * GRID_SIZE, f) != GRID_SIZE * GRID_SIZE ||
      fread(&game->score, sizeof(int), 1, f) != 1 ||
      fread(&game->move_count, sizeof(int), 1, f) != 1 ||
      fread(&game->last_score_gain, sizeof(int), 1, f) != 1) {
    fclose(f);
    return false;
  }

  int st = 0;
  int won = 0;
  if (fread(&st, sizeof(int), 1, f) != 1 ||
      fread(&won, sizeof(int), 1, f) != 1) {
    fclose(f);
    return false;
  }
  game->state = (PlayState)st;
  game->won_acknowledged = (won != 0);

  if (fread(&game->history_count, sizeof(int), 1, f) != 1) {
    fclose(f);
    return false;
  }

  if (game->history_count < 0 || game->history_count > MAX_UNDO_STATES) {
    game->history_count = 0;
  } else if (game->history_count > 0) {
    if ((int)fread(game->history, sizeof(BoardSnapshot), game->history_count, f) != game->history_count) {
      game->history_count = 0;
    }
  }

  fclose(f);

  if (!game_can_move(game)) {
    game_delete_save();
    return false;
  }

  return true;
}

void game_delete_save(void) {
  remove(get_save_path());
}

void game_reset(GameState *game) {
  game_delete_save();
  memset(game->board, 0, sizeof(game->board));
  game->score = 0;
  game->move_count = 0;
  game->last_score_gain = 0;
  game->state = STATE_PLAYING;
  game->won_acknowledged = false;
  game->history_count = 0;

  game_spawn_tile(game);
  game_spawn_tile(game);
  game_save(game);
}

void game_init(GameState *game) {
  memset(game, 0, sizeof(GameState));
  game_load_high_score(game);
  if (!game_load(game)) {
    game_reset(game);
  }
}

bool game_can_move(const GameState *game) {
  // Check for any empty cells
  for (int r = 0; r < GRID_SIZE; r++) {
    for (int c = 0; c < GRID_SIZE; c++) {
      if (game->board[r][c] == 0) {
        return true;
      }
    }
  }

  // Check for horizontal and vertical merges
  for (int r = 0; r < GRID_SIZE; r++) {
    for (int c = 0; c < GRID_SIZE; c++) {
      int val = game->board[r][c];
      if (c + 1 < GRID_SIZE && game->board[r][c + 1] == val) {
        return true;
      }
      if (r + 1 < GRID_SIZE && game->board[r + 1][c] == val) {
        return true;
      }
    }
  }

  return false;
}

bool game_has_won(const GameState *game) {
  for (int r = 0; r < GRID_SIZE; r++) {
    for (int c = 0; c < GRID_SIZE; c++) {
      if (game->board[r][c] >= WIN_TARGET) {
        return true;
      }
    }
  }
  return false;
}

// Slide and merge a 1D line of 4 tiles to the left
static int slide_line_left(int line[GRID_SIZE]) {
  int gain = 0;
  int temp[GRID_SIZE] = {0};
  int count = 0;

  // Filter non-zeros
  for (int i = 0; i < GRID_SIZE; i++) {
    if (line[i] != 0) {
      temp[count++] = line[i];
    }
  }

  // Merge adjacent equal pairs
  for (int i = 0; i < count - 1; i++) {
    if (temp[i] == temp[i + 1]) {
      temp[i] *= 2;
      gain += temp[i];
      temp[i + 1] = 0;
      i++;
    }
  }

  // Pack non-zeros back into line
  memset(line, 0, sizeof(int) * GRID_SIZE);
  int write_idx = 0;
  for (int i = 0; i < GRID_SIZE; i++) {
    if (temp[i] != 0) {
      line[write_idx++] = temp[i];
    }
  }

  return gain;
}

static void push_history(GameState *game, const int prev_board[GRID_SIZE][GRID_SIZE]) {
  if (game->history_count < MAX_UNDO_STATES) {
    memcpy(game->history[game->history_count].board, prev_board, sizeof(game->board));
    game->history[game->history_count].score = game->score;
    game->history_count++;
  } else {
    // Shift left to make room for newest state
    for (int i = 0; i < MAX_UNDO_STATES - 1; i++) {
      game->history[i] = game->history[i + 1];
    }
    memcpy(game->history[MAX_UNDO_STATES - 1].board, prev_board, sizeof(game->board));
    game->history[MAX_UNDO_STATES - 1].score = game->score;
  }
}

bool game_undo(GameState *game) {
  if (game->history_count <= 0) {
    return false;
  }

  game->history_count--;
  memcpy(game->board, game->history[game->history_count].board, sizeof(game->board));
  game->score = game->history[game->history_count].score;
  game->last_score_gain = 0;
  game->state = STATE_PLAYING;
  game_save(game);
  return true;
}

bool game_slide(GameState *game, MoveDirection dir) {
  if (dir == DIR_NONE || game->state == STATE_GAME_OVER) {
    return false;
  }

  int prev_board[GRID_SIZE][GRID_SIZE];
  memcpy(prev_board, game->board, sizeof(game->board));
  int total_gain = 0;

  switch (dir) {
    case DIR_LEFT: {
      for (int r = 0; r < GRID_SIZE; r++) {
        total_gain += slide_line_left(game->board[r]);
      }
      break;
    }

    case DIR_RIGHT: {
      for (int r = 0; r < GRID_SIZE; r++) {
        int line[GRID_SIZE];
        for (int c = 0; c < GRID_SIZE; c++) {
          line[c] = game->board[r][GRID_SIZE - 1 - c];
        }
        total_gain += slide_line_left(line);
        for (int c = 0; c < GRID_SIZE; c++) {
          game->board[r][GRID_SIZE - 1 - c] = line[c];
        }
      }
      break;
    }

    case DIR_UP: {
      for (int c = 0; c < GRID_SIZE; c++) {
        int line[GRID_SIZE];
        for (int r = 0; r < GRID_SIZE; r++) {
          line[r] = game->board[r][c];
        }
        total_gain += slide_line_left(line);
        for (int r = 0; r < GRID_SIZE; r++) {
          game->board[r][c] = line[r];
        }
      }
      break;
    }

    case DIR_DOWN: {
      for (int c = 0; c < GRID_SIZE; c++) {
        int line[GRID_SIZE];
        for (int r = 0; r < GRID_SIZE; r++) {
          line[r] = game->board[GRID_SIZE - 1 - r][c];
        }
        total_gain += slide_line_left(line);
        for (int r = 0; r < GRID_SIZE; r++) {
          game->board[GRID_SIZE - 1 - r][c] = line[r];
        }
      }
      break;
    }

    default:
      return false;
  }

  // Check if anything changed
  bool changed = false;
  for (int r = 0; r < GRID_SIZE; r++) {
    for (int c = 0; c < GRID_SIZE; c++) {
      if (game->board[r][c] != prev_board[r][c]) {
        changed = true;
        break;
      }
    }
    if (changed) break;
  }

  if (!changed) {
    return false;
  }

  // Save previous state to history
  push_history(game, prev_board);

  game->score += total_gain;
  game->last_score_gain = total_gain;
  game->move_count++;

  if (game->score > game->high_score) {
    game->high_score = game->score;
    game_save_high_score(game);
  }

  // Spawn new random tile
  game_spawn_tile(game);

  // Check for 2048 win
  if (!game->won_acknowledged && game_has_won(game)) {
    game->state = STATE_WON;
    game_save(game);
  } else if (!game_can_move(game)) {
    game->state = STATE_GAME_OVER;
    game_delete_save();
  } else {
    game_save(game);
  }

  return true;
}
