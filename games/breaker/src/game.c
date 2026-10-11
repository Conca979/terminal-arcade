#include "game.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../../include/scores.h"

#define PI 3.14159265358979323846f
#define SAVE_MAGIC 0x4B524242 // "BBRK"
#define SAVE_VERSION 1

static int load_high_score(void) {
  return arcade_score_get("breaker", "high_score");
}

static void save_high_score(int score) {
  arcade_score_set("breaker", "high_score", score);
}

void game_save(const GameState *game) {
  FILE *f = fopen(SAVE_FILE, "wb");
  if (!f) return;

  uint32_t magic = SAVE_MAGIC;
  uint32_t ver = SAVE_VERSION;
  fwrite(&magic, sizeof(uint32_t), 1, f);
  fwrite(&ver, sizeof(uint32_t), 1, f);

  fwrite(&game->round, sizeof(int), 1, f);
  fwrite(&game->score, sizeof(int), 1, f);
  fwrite(&game->total_balls, sizeof(int), 1, f);
  fwrite(&game->launcher_x, sizeof(float), 1, f);
  fwrite(&game->aim_angle, sizeof(float), 1, f);
  fwrite(game->grid, sizeof(Brick), BRICK_ROWS * BRICK_COLS, f);

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

  if (fread(&game->round, sizeof(int), 1, f) != 1 ||
      fread(&game->score, sizeof(int), 1, f) != 1 ||
      fread(&game->total_balls, sizeof(int), 1, f) != 1 ||
      fread(&game->launcher_x, sizeof(float), 1, f) != 1 ||
      fread(&game->aim_angle, sizeof(float), 1, f) != 1 ||
      fread(game->grid, sizeof(Brick), BRICK_ROWS * BRICK_COLS, f) != (BRICK_ROWS * BRICK_COLS)) {
    fclose(f);
    return false;
  }

  fclose(f);
  game->next_launcher_x = game->launcher_x;
  game->phase = PHASE_AIMING;
  game->balls_active = 0;
  game->balls_to_launch = 0;
  game->collected_balls = 0;
  game->turbo_mode = false;
  game->resumed_game = true;
  game_calculate_trajectory(game);
  return true;
}

void game_delete_save(void) {
  remove(SAVE_FILE);
}

void game_spawn_new_row(GameState *game) {
  // Spawn a fresh row at row 0
  int items_spawned = 0;
  bool orb_spawned = false;

  for (int c = 0; c < BRICK_COLS; c++) {
    int roll = rand() % 100;
    Brick *b = &game->grid[0][c];

    if (roll < 45) { // 45% normal brick
      b->type = BRICK_NORMAL;
      b->hp = game->round;
      b->max_hp = game->round;
      b->flash_timer = 0;
      items_spawned++;
    } else if (roll < 60 && !orb_spawned) { // 15% extra ball orb
      b->type = BRICK_ADD_BALL;
      b->hp = 1;
      b->max_hp = 1;
      b->flash_timer = 0;
      orb_spawned = true;
      items_spawned++;
    } else if (roll < 70) { // 10% bomb or laser
      int special = rand() % 3;
      if (special == 0) b->type = BRICK_BOMB;
      else if (special == 1) b->type = BRICK_LASER_ROW;
      else b->type = BRICK_LASER_COL;
      b->hp = (game->round > 2) ? (game->round / 2 + 1) : 1;
      b->max_hp = b->hp;
      b->flash_timer = 0;
      items_spawned++;
    } else {
      b->type = BRICK_NONE;
      b->hp = 0;
      b->max_hp = 0;
      b->flash_timer = 0;
    }
  }

  // Ensure at least 1 extra ball orb spawns if none rolled
  if (!orb_spawned) {
    int empty_cols[BRICK_COLS];
    int empty_count = 0;
    for (int c = 0; c < BRICK_COLS; c++) {
      if (game->grid[0][c].type == BRICK_NONE) {
        empty_cols[empty_count++] = c;
      }
    }
    if (empty_count > 0) {
      int pick = empty_cols[rand() % empty_count];
      game->grid[0][pick].type = BRICK_ADD_BALL;
      game->grid[0][pick].hp = 1;
      game->grid[0][pick].max_hp = 1;
      items_spawned++;
    }
  }
}

void game_calculate_trajectory(GameState *game) {
  game->laser_guide.count = 0;

  float px = game->launcher_x;
  float py = ARENA_HEIGHT - 1.0f;
  float vx = cosf(game->aim_angle);
  float vy = sinf(game->aim_angle);

  // Normalize velocity
  float len = sqrtf(vx * vx + vy * vy);
  if (len > 0.0001f) {
    vx /= len;
    vy /= len;
  }

  game->laser_guide.points[game->laser_guide.count++] = (Vec2){px, py};

  const float step = 1.0f;
  int bounces = 0;

  for (int s = 0; s < 50 && game->laser_guide.count < 60; s++) {
    px += vx * step;
    py += vy * step;

    // Left wall bounce
    if (px <= 0.5f) {
      px = 0.5f;
      vx = -vx;
      bounces++;
      if (bounces > 2) break;
    }
    // Right wall bounce
    else if (px >= ARENA_WIDTH - 0.5f) {
      px = ARENA_WIDTH - 0.5f;
      vx = -vx;
      bounces++;
      if (bounces > 2) break;
    }

    // Top wall bounce
    if (py <= 0.5f) {
      py = 0.5f;
      vy = -vy;
      bounces++;
      if (bounces > 2) break;
    }

    // Check brick collision along laser
    int c = (int)(px / BRICK_WIDTH);
    int r = (int)py;
    if (c >= 0 && c < BRICK_COLS && r >= 0 && r < BRICK_ROWS) {
      if (game->grid[r][c].type == BRICK_NORMAL || game->grid[r][c].type == BRICK_BOMB) {
        game->laser_guide.points[game->laser_guide.count++] = (Vec2){px, py};
        break;
      }
    }

    // Store trajectory dots at interval
    if (s % 2 == 0) {
      game->laser_guide.points[game->laser_guide.count++] = (Vec2){px, py};
    }
  }
}

void game_reset(GameState *game) {
  memset(game->grid, 0, sizeof(game->grid));
  memset(game->balls, 0, sizeof(game->balls));

  game->round = 1;
  game->score = 0;
  game->total_balls = 1;
  game->balls_to_launch = 0;
  game->balls_active = 0;
  game->collected_balls = 0;
  game->launcher_x = ARENA_WIDTH / 2.0f;
  game->next_launcher_x = game->launcher_x;
  game->first_ball_landed = false;
  game->aim_angle = -1.5707963f; // -PI / 2
  game->phase = PHASE_AIMING;
  game->turbo_mode = false;
  game->resumed_game = false;
  game->last_launch_time = 0;
  game->mouse_aiming = false;

  game_delete_save();
  game_spawn_new_row(game);
  game_calculate_trajectory(game);
  game_save(game);
}

void game_init(GameState *game) {
  memset(game, 0, sizeof(GameState));
  game->high_score = load_high_score();

  if (!game_load(game)) {
    game_reset(game);
  }
}

void game_set_aim_angle(GameState *game, float angle_rad) {
  // Clamp between -3.00 (-172 deg) and -0.14 (-8 deg)
  if (angle_rad < -3.00f) angle_rad = -3.00f;
  if (angle_rad > -0.14f) angle_rad = -0.14f;

  game->aim_angle = angle_rad;
  game_calculate_trajectory(game);
}

void game_set_aim_target(GameState *game, float target_x, float target_y) {
  float dx = target_x - game->launcher_x;
  float dy = target_y - (ARENA_HEIGHT - 1.0f);

  if (dy < -0.5f) { // Aiming upward
    float angle = atan2f(dy, dx);
    game_set_aim_angle(game, angle);
  }
}

void game_fire(GameState *game, unsigned long long current_time) {
  if (game->phase != PHASE_AIMING) return;

  game->phase = PHASE_FIRING;
  game->balls_to_launch = game->total_balls;
  game->balls_active = 0;
  game->collected_balls = 0;
  game->first_ball_landed = false;
  game->last_launch_time = current_time;
  game->turbo_mode = false;

  for (int i = 0; i < MAX_BALLS; i++) {
    game->balls[i].active = false;
    game->balls[i].landed = false;
  }
}

static void trigger_bomb(GameState *game, int center_r, int center_c, int damage) {
  for (int dr = -1; dr <= 1; dr++) {
    for (int dc = -1; dc <= 1; dc++) {
      int r = center_r + dr;
      int c = center_c + dc;
      if (r >= 0 && r < BRICK_ROWS && c >= 0 && c < BRICK_COLS) {
        Brick *b = &game->grid[r][c];
        if (b->type != BRICK_NONE) {
          b->hp -= damage;
          if (b->hp <= 0) {
            b->type = BRICK_NONE;
            game->score += 30;
          }
        }
      }
    }
  }
}

static void advance_round(GameState *game) {
  // Check if any brick is about to cross the danger line
  for (int c = 0; c < BRICK_COLS; c++) {
    if (game->grid[DANGER_ROW - 1][c].type == BRICK_NORMAL ||
        game->grid[DANGER_ROW - 1][c].type == BRICK_BOMB) {
      game->phase = PHASE_GAME_OVER;
      game_delete_save();
      if (game->score > game->high_score) {
        game->high_score = game->score;
        save_high_score(game->high_score);
      }
      return;
    }
  }

  // Shift all bricks down by 1 row
  for (int r = BRICK_ROWS - 1; r > 0; r--) {
    for (int c = 0; c < BRICK_COLS; c++) {
      game->grid[r][c] = game->grid[r - 1][c];
    }
  }

  // Clear row 0 and spawn new row
  for (int c = 0; c < BRICK_COLS; c++) {
    memset(&game->grid[0][c], 0, sizeof(Brick));
  }

  game->round++;
  game->score += game->round * 10;
  if (game->score > game->high_score) {
    game->high_score = game->score;
    save_high_score(game->high_score);
  }

  game->total_balls += game->collected_balls;
  if (game->total_balls > MAX_BALLS) game->total_balls = MAX_BALLS;
  game->collected_balls = 0;

  // Move launcher to new landing location
  game->launcher_x = game->next_launcher_x;
  if (game->launcher_x < 2.0f) game->launcher_x = 2.0f;
  if (game->launcher_x > ARENA_WIDTH - 2.0f) game->launcher_x = ARENA_WIDTH - 2.0f;

  game_spawn_new_row(game);
  game->phase = PHASE_AIMING;
  game->turbo_mode = false;
  game_calculate_trajectory(game);

  // Auto-save state
  game_save(game);
}

void game_update(GameState *game, unsigned long long current_time) {
  if (game->phase != PHASE_FIRING) {
    return;
  }

  float speed = game->turbo_mode ? TURBO_SPEED : BALL_SPEED;
  unsigned long long launch_interval = game->turbo_mode ? (LAUNCH_DELAY_MS / 3) : LAUNCH_DELAY_MS;

  // 1. Launch stream of balls
  if (game->balls_to_launch > 0) {
    if (current_time - game->last_launch_time >= launch_interval) {
      for (int i = 0; i < MAX_BALLS; i++) {
        if (!game->balls[i].active && !game->balls[i].landed) {
          game->balls[i].x = game->launcher_x;
          game->balls[i].y = ARENA_HEIGHT - 1.2f;
          game->balls[i].vx = cosf(game->aim_angle) * speed;
          game->balls[i].vy = sinf(game->aim_angle) * speed;
          game->balls[i].active = true;
          game->balls[i].landed = false;
          game->balls_to_launch--;
          game->balls_active++;
          game->last_launch_time = current_time;
          break;
        }
      }
    }
  }

  // 2. Physics Sub-steps
  const int SUBSTEPS = 2;
  float dt = 1.0f / (float)SUBSTEPS;

  for (int step = 0; step < SUBSTEPS; step++) {
    for (int i = 0; i < MAX_BALLS; i++) {
      Ball *b = &game->balls[i];
      if (!b->active) continue;

      float prev_x = b->x;
      float prev_y = b->y;

      b->x += b->vx * dt;
      b->y += b->vy * dt;

      // Left wall bounce
      if (b->x <= 0.6f) {
        b->x = 0.6f;
        b->vx = fabsf(b->vx);
      }
      // Right wall bounce
      else if (b->x >= ARENA_WIDTH - 0.6f) {
        b->x = ARENA_WIDTH - 0.6f;
        b->vx = -fabsf(b->vx);
      }

      // Top wall bounce
      if (b->y <= 0.6f) {
        b->y = 0.6f;
        b->vy = fabsf(b->vy);
      }

      // Brick collision
      int col = (int)(b->x / BRICK_WIDTH);
      int row = (int)b->y;

      if (col >= 0 && col < BRICK_COLS && row >= 0 && row < BRICK_ROWS) {
        Brick *brk = &game->grid[row][col];
        if (brk->type == BRICK_ADD_BALL) {
          // Collect orb
          game->collected_balls++;
          game->score += 25;
          brk->type = BRICK_NONE;
        } else if (brk->type != BRICK_NONE && brk->hp > 0) {
          // Bounce off brick edge
          int prev_col = (int)(prev_x / BRICK_WIDTH);
          int prev_row = (int)prev_y;

          if (prev_col != col) {
            b->vx = -b->vx;
          } else if (prev_row != row) {
            b->vy = -b->vy;
          } else {
            b->vy = -b->vy;
          }

          brk->hp--;
          brk->flash_timer = 2;
          game->score += 10;

          if (brk->hp <= 0) {
            if (brk->type == BRICK_BOMB) {
              trigger_bomb(game, row, col, game->round / 2 + 1);
            } else if (brk->type == BRICK_LASER_ROW) {
              for (int c = 0; c < BRICK_COLS; c++) {
                if (game->grid[row][c].type != BRICK_NONE) {
                  game->grid[row][c].hp -= (game->round / 2 + 1);
                  if (game->grid[row][c].hp <= 0) game->grid[row][c].type = BRICK_NONE;
                }
              }
            } else if (brk->type == BRICK_LASER_COL) {
              for (int r = 0; r < BRICK_ROWS; r++) {
                if (game->grid[r][col].type != BRICK_NONE) {
                  game->grid[r][col].hp -= (game->round / 2 + 1);
                  if (game->grid[r][col].hp <= 0) game->grid[r][col].type = BRICK_NONE;
                }
              }
            }
            brk->type = BRICK_NONE;
            game->score += 50;
          }
        }
      }

      // Bottom boundary landing
      if (b->y >= ARENA_HEIGHT - 1.0f) {
        b->y = ARENA_HEIGHT - 1.0f;
        b->active = false;
        b->landed = true;
        game->balls_active--;

        if (!game->first_ball_landed) {
          game->first_ball_landed = true;
          game->next_launcher_x = b->x;
          if (game->next_launcher_x < 2.0f) game->next_launcher_x = 2.0f;
          if (game->next_launcher_x > ARENA_WIDTH - 2.0f) game->next_launcher_x = ARENA_WIDTH - 2.0f;
        }
      }
    }
  }

  // Check if round finished
  if (game->balls_to_launch == 0 && game->balls_active == 0) {
    advance_round(game);
  }
}

void game_handle_action(GameState *game, GameAction action, int param1, int param2, unsigned long long current_time) {
  if (action == ACT_QUIT) {
    game->phase = PHASE_EXIT;
    return;
  }

  if (action == ACT_RESTART) {
    game_reset(game);
    return;
  }

  if (game->phase == PHASE_GAME_OVER) {
    return;
  }

  if (game->phase == PHASE_AIMING) {
    if (action == ACT_AIM_LEFT) {
      game_set_aim_angle(game, game->aim_angle - 0.045f);
    } else if (action == ACT_AIM_RIGHT) {
      game_set_aim_angle(game, game->aim_angle + 0.045f);
    } else if (action == ACT_MOUSE_MOVE) {
      game->mouse_term_x = param1;
      game->mouse_term_y = param2;
      // In render.c: Arena start column is 2, start row is 4
      float arena_x = (float)(param1 - 3);
      float arena_y = (float)(param2 - 4);
      game_set_aim_target(game, arena_x, arena_y);
    } else if (action == ACT_FIRE) {
      if (param1 > 0 || param2 > 0) {
        float arena_x = (float)(param1 - 3);
        float arena_y = (float)(param2 - 4);
        game_set_aim_target(game, arena_x, arena_y);
      }
      game_fire(game, current_time);
    }
  } else if (game->phase == PHASE_FIRING) {
    if (action == ACT_TURBO || action == ACT_FIRE) {
      game->turbo_mode = true;
    }
  }
}
