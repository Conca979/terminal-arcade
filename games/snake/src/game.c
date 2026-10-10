#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

static Position find_free_cell(const GameState *game) {
  Position free_cells[MAX_SNAKE_LEN];
  int count = 0;

  for (int y = 0; y < ARENA_HEIGHT; y++) {
    for (int x = 0; x < ARENA_WIDTH; x++) {
      bool occupied = false;
      for (int i = 0; i < game->length; i++) {
        if (game->body[i].x == x && game->body[i].y == y) {
          occupied = true;
          break;
        }
      }
      if (!occupied && game->regular_food.type != FOOD_NONE &&
          game->regular_food.pos.x == x && game->regular_food.pos.y == y) {
        occupied = true;
      }
      if (!occupied && game->bonus_food.type != FOOD_NONE &&
          game->bonus_food.pos.x == x && game->bonus_food.pos.y == y) {
        occupied = true;
      }
      if (!occupied) {
        free_cells[count].x = x;
        free_cells[count].y = y;
        count++;
      }
    }
  }

  if (count == 0) {
    Position fallback = {ARENA_WIDTH / 2, ARENA_HEIGHT / 2};
    return fallback;
  }

  return free_cells[rand() % count];
}

static void spawn_regular_food(GameState *game) {
  game->regular_food.pos = find_free_cell(game);
  game->regular_food.type = FOOD_DATA;
}

static void spawn_bonus_food(GameState *game, unsigned long long current_time) {
  int r = rand() % 3;
  FoodType type = FOOD_CORE;
  if (r == 1) type = FOOD_OVERCLOCK;
  else if (r == 2) type = FOOD_PHASE;

  game->bonus_food.pos = find_free_cell(game);
  game->bonus_food.type = type;
  game->bonus_food.spawn_time = current_time;
  game->bonus_food.expire_time = current_time + BONUS_DURATION_MS;

  // Next bonus food in 14-22 seconds
  game->next_bonus_spawn_time = current_time + 14000 + (rand() % 8000);
}

int game_get_tick_interval(const GameState *game) {
  int reduction = (int)(game->length * 1.5);
  int interval = BASE_TICK_MS - reduction;
  if (game->overclock_active) {
    interval -= 15;
  }
  return (interval < MIN_TICK_MS) ? MIN_TICK_MS : interval;
}

void game_reset(GameState *game) {
  memset(game->body, 0, sizeof(game->body));
  int start_x = ARENA_WIDTH / 4;
  int start_y = ARENA_HEIGHT / 2;

  // Start with 4 segments
  game->length = 4;
  for (int i = 0; i < game->length; i++) {
    game->body[i].x = start_x - i;
    game->body[i].y = start_y;
  }

  game->dir = DIR_RIGHT;
  game->next_dir = DIR_RIGHT;

  game->overclock_active = false;
  game->overclock_expire = 0;
  game->phase_active = false;
  game->phase_expire = 0;

  game->score = 0;
  game->state = STATE_PLAYING;
  game->last_move_time = 0;
  game->next_bonus_spawn_time = 12000;

  game->bonus_food.type = FOOD_NONE;
  spawn_regular_food(game);
}

void game_init(GameState *game) {
  game_load_high_score(game);
  game->wrap_mode = false; // Default: classic lethal walls
  game_reset(game);
}

void game_handle_action(GameState *game, GameAction action, unsigned long long current_time) {
  (void)current_time;

  if (action == ACT_QUIT) {
    game->state = STATE_EXIT;
    return;
  }

  if (action == ACT_RESTART) {
    game_reset(game);
    return;
  }

  if (action == ACT_PAUSE) {
    if (game->state == STATE_PLAYING) {
      game->state = STATE_PAUSED;
    } else if (game->state == STATE_PAUSED) {
      game->state = STATE_PLAYING;
    }
    return;
  }

  if (action == ACT_TOGGLE_WRAP) {
    game->wrap_mode = !game->wrap_mode;
    return;
  }

  if (game->state != STATE_PLAYING) {
    return;
  }

  switch (action) {
    case ACT_UP:
      if (game->dir != DIR_DOWN) {
        game->next_dir = DIR_UP;
      }
      break;

    case ACT_DOWN:
      if (game->dir != DIR_UP) {
        game->next_dir = DIR_DOWN;
      }
      break;

    case ACT_LEFT:
      if (game->dir != DIR_RIGHT) {
        game->next_dir = DIR_LEFT;
      }
      break;

    case ACT_RIGHT:
      if (game->dir != DIR_LEFT) {
        game->next_dir = DIR_RIGHT;
      }
      break;

    default:
      break;
  }
}

void game_update(GameState *game, unsigned long long current_time) {
  if (game->state != STATE_PLAYING) {
    return;
  }

  // Check buff expirations
  if (game->overclock_active && current_time >= game->overclock_expire) {
    game->overclock_active = false;
  }
  if (game->phase_active && current_time >= game->phase_expire) {
    game->phase_active = false;
  }

  // Check bonus food expiration
  if (game->bonus_food.type != FOOD_NONE && current_time >= game->bonus_food.expire_time) {
    game->bonus_food.type = FOOD_NONE;
  }

  // Spawn bonus food if interval elapsed
  if (game->bonus_food.type == FOOD_NONE && current_time >= game->next_bonus_spawn_time) {
    spawn_bonus_food(game, current_time);
  }

  // Check movement tick
  int tick = game_get_tick_interval(game);
  if (current_time - game->last_move_time < (unsigned long long)tick) {
    return;
  }
  game->last_move_time = current_time;

  game->dir = game->next_dir;

  Position new_head = game->body[0];
  switch (game->dir) {
    case DIR_UP:    new_head.y--; break;
    case DIR_DOWN:  new_head.y++; break;
    case DIR_LEFT:  new_head.x--; break;
    case DIR_RIGHT: new_head.x++; break;
  }

  // Boundary check
  bool out_of_bounds = (new_head.x < 0 || new_head.x >= ARENA_WIDTH ||
                        new_head.y < 0 || new_head.y >= ARENA_HEIGHT);

  if (out_of_bounds) {
    if (game->wrap_mode || game->phase_active) {
      new_head.x = (new_head.x + ARENA_WIDTH) % ARENA_WIDTH;
      new_head.y = (new_head.y + ARENA_HEIGHT) % ARENA_HEIGHT;
    } else {
      game->state = STATE_GAME_OVER;
      return;
    }
  }

  // Self-collision check
  // (tail moves away unless we're eating food)
  for (int i = 0; i < game->length - 1; i++) {
    if (game->body[i].x == new_head.x && game->body[i].y == new_head.y) {
      game->state = STATE_GAME_OVER;
      return;
    }
  }

  // Food check
  int growth = 0;
  int multiplier = game->overclock_active ? 2 : 1;

  if (new_head.x == game->regular_food.pos.x && new_head.y == game->regular_food.pos.y) {
    game->score += 10 * multiplier;
    growth += 1;
    spawn_regular_food(game);
  }

  if (game->bonus_food.type != FOOD_NONE &&
      new_head.x == game->bonus_food.pos.x && new_head.y == game->bonus_food.pos.y) {
    switch (game->bonus_food.type) {
      case FOOD_CORE:
        game->score += 50 * multiplier;
        growth += 2;
        break;

      case FOOD_OVERCLOCK:
        game->score += 25 * multiplier;
        game->overclock_active = true;
        game->overclock_expire = current_time + BUFF_DURATION_MS;
        break;

      case FOOD_PHASE:
        game->score += 25 * multiplier;
        game->phase_active = true;
        game->phase_expire = current_time + BUFF_DURATION_MS;
        break;

      default:
        break;
    }
    game->bonus_food.type = FOOD_NONE;
  }

  if (game->score > game->high_score) {
    game->high_score = game->score;
    game_save_high_score(game);
  }

  // Move snake body
  int new_length = game->length + growth;
  if (new_length > MAX_SNAKE_LEN) {
    new_length = MAX_SNAKE_LEN;
  }

  for (int i = new_length - 1; i > 0; i--) {
    if (i < game->length) {
      game->body[i] = game->body[i - 1];
    } else {
      game->body[i] = game->body[game->length - 1];
    }
  }
  game->body[0] = new_head;
  game->length = new_length;
}
