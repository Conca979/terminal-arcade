#include "game.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../../include/scores.h"

#define SAVE_MAGIC_FLAPPY 0x464C5059 // "FLPY"
#define SAVE_VERSION_FLAPPY 1

static const char *get_save_path(void) {
  FILE *f = fopen("games/flappy/include/game.h", "r");
  if (f) {
    fclose(f);
    return "games/flappy/savegame.dat";
  }
  return "savegame.dat";
}

MedalTier game_get_medal(int score) {
  if (score >= MEDAL_PLATINUM) return MEDAL_PLATINUM_TIER;
  if (score >= MEDAL_GOLD)     return MEDAL_GOLD_TIER;
  if (score >= MEDAL_SILVER)   return MEDAL_SILVER_TIER;
  if (score >= MEDAL_BRONZE)   return MEDAL_BRONZE_TIER;
  return MEDAL_NONE;
}

void game_load_high_score(GameState *game) {
  game->high_score = arcade_score_get("flappy", "high_score");
}

void game_save_high_score(const GameState *game) {
  arcade_score_set("flappy", "high_score", game->high_score);
}

void game_delete_save(void) {
  remove(get_save_path());
}

void game_save(const GameState *game) {
  if (game->state == STATE_GAME_OVER || game->state == STATE_EXIT) {
    return;
  }

  FILE *f = fopen(get_save_path(), "wb");
  if (!f) return;

  uint32_t magic = SAVE_MAGIC_FLAPPY;
  uint32_t ver = SAVE_VERSION_FLAPPY;
  fwrite(&magic, sizeof(uint32_t), 1, f);
  fwrite(&ver, sizeof(uint32_t), 1, f);

  fwrite(&game->bird, sizeof(Bird), 1, f);
  fwrite(game->pipes, sizeof(Pipe), MAX_PIPES, f);
  fwrite(game->clouds, sizeof(Cloud), MAX_CLOUDS, f);
  fwrite(&game->ground_offset, sizeof(float), 1, f);
  fwrite(&game->score, sizeof(int), 1, f);
  fwrite(&game->high_score, sizeof(int), 1, f);

  int st = (game->state == STATE_PLAYING) ? (int)STATE_PAUSED : (int)game->state;
  fwrite(&st, sizeof(int), 1, f);

  fclose(f);
}

bool game_load(GameState *game) {
  FILE *f = fopen(get_save_path(), "rb");
  if (!f) return false;

  uint32_t magic = 0, ver = 0;
  if (fread(&magic, sizeof(uint32_t), 1, f) != 1 || magic != SAVE_MAGIC_FLAPPY) {
    fclose(f);
    return false;
  }
  if (fread(&ver, sizeof(uint32_t), 1, f) != 1 || ver != SAVE_VERSION_FLAPPY) {
    fclose(f);
    return false;
  }

  if (fread(&game->bird, sizeof(Bird), 1, f) != 1 ||
      fread(game->pipes, sizeof(Pipe), MAX_PIPES, f) != MAX_PIPES ||
      fread(game->clouds, sizeof(Cloud), MAX_CLOUDS, f) != MAX_CLOUDS ||
      fread(&game->ground_offset, sizeof(float), 1, f) != 1 ||
      fread(&game->score, sizeof(int), 1, f) != 1 ||
      fread(&game->high_score, sizeof(int), 1, f) != 1) {
    fclose(f);
    return false;
  }

  int st = 0;
  if (fread(&st, sizeof(int), 1, f) != 1) {
    fclose(f);
    return false;
  }
  fclose(f);

  game->state = (st == STATE_GAME_OVER) ? STATE_READY : (PlayState)st;
  game->medal = game_get_medal(game->score);
  game->state_timer = 0.0f;
  game->is_new_high = false;
  game->flash_timer_ms = 0;

  return true;
}

bool game_check_collision(const GameState *game) {
  int bird_row = (int)roundf(game->bird.y);

  // 1. Ground collision
  if (bird_row >= SKY_HEIGHT - 1) {
    return true;
  }

  // 2. Pipe collision
  float bird_left = BIRD_X;
  float bird_right = BIRD_X + (float)BIRD_WIDTH;

  for (int i = 0; i < MAX_PIPES; i++) {
    float pipe_left = game->pipes[i].x;
    float pipe_right = game->pipes[i].x + (float)PIPE_WIDTH;

    // Check horizontal overlap
    if (bird_right > pipe_left && bird_left < pipe_right) {
      int gap_top = game->pipes[i].gap_y;
      int gap_bottom = gap_top + PIPE_GAP;

      // Collides with top pipe or bottom pipe
      if (bird_row < gap_top || bird_row >= gap_bottom) {
        return true;
      }
    }
  }

  return false;
}

void game_reset(GameState *game) {
  game_delete_save();

  game->bird.y = BIRD_START_Y;
  game->bird.vy = 0.0f;
  game->bird.flap_anim_timer = 0.0f;

  // Initialize spaced pipes starting off-screen right
  float start_x = (float)PLAYFIELD_WIDTH + 6.0f;
  for (int i = 0; i < MAX_PIPES; i++) {
    game->pipes[i].x = start_x + (float)i * PIPE_SPACING;
    game->pipes[i].gap_y = 2 + (rand() % (SKY_HEIGHT - PIPE_GAP - 3));
    game->pipes[i].passed = false;
  }

  // Initialize clouds with varied positions, heights, and speeds
  for (int i = 0; i < MAX_CLOUDS; i++) {
    game->clouds[i].x = (float)(i * 14 + (rand() % 6));
    game->clouds[i].y = (float)(1 + (i % 5));
    game->clouds[i].speed = 3.0f + (float)(rand() % 4) * 0.8f;
    game->clouds[i].type = i % 3;
  }

  game->ground_offset = 0.0f;
  game->score = 0;
  game->medal = MEDAL_NONE;
  game->state = STATE_READY;
  game->state_timer = 0.0f;
  game->is_new_high = false;
  game->flash_timer_ms = 0;
}

void game_init(GameState *game) {
  memset(game, 0, sizeof(GameState));
  game_load_high_score(game);

  if (!game_load(game)) {
    game_reset(game);
  }
}

void game_handle_action(GameState *game, GameAction action) {
  switch (action) {
    case ACT_FLAP:
      if (game->state == STATE_READY) {
        game->state = STATE_PLAYING;
        game->bird.vy = FLAP_IMPULSE;
        game->bird.flap_anim_timer = 0.16f;
      } else if (game->state == STATE_PLAYING) {
        game->bird.vy = FLAP_IMPULSE;
        game->bird.flap_anim_timer = 0.16f;
      } else if (game->state == STATE_GAME_OVER) {
        if (game->state_timer > 0.35f) {
          game_reset(game);
        }
      }
      break;

    case ACT_PAUSE:
      if (game->state == STATE_PLAYING) {
        game->state = STATE_PAUSED;
      } else if (game->state == STATE_PAUSED) {
        game->state = STATE_PLAYING;
      }
      break;

    case ACT_RESTART:
      game_reset(game);
      break;

    case ACT_QUIT:
      if (game->state == STATE_PLAYING || game->state == STATE_READY) {
        game_save(game);
      }
      game->state = STATE_EXIT;
      break;

    default:
      break;
  }
}

void game_update(GameState *game, float dt) {
  if (dt <= 0.0f) return;
  if (dt > 0.05f) dt = 0.05f;

  if (game->flash_timer_ms > 0) {
    game->flash_timer_ms -= (int)(dt * 1000.0f);
    if (game->flash_timer_ms < 0) {
      game->flash_timer_ms = 0;
    }
  }

  if (game->bird.flap_anim_timer > 0.0f) {
    game->bird.flap_anim_timer -= dt;
  }

  // Update background clouds across all non-paused states
  if (game->state != STATE_PAUSED) {
    for (int i = 0; i < MAX_CLOUDS; i++) {
      game->clouds[i].x -= game->clouds[i].speed * dt;
      if (game->clouds[i].x < -12.0f) {
        game->clouds[i].x = (float)PLAYFIELD_WIDTH + 4.0f + (float)(rand() % 8);
        game->clouds[i].y = (float)(1 + (rand() % 5));
      }
    }
  }

  if (game->state == STATE_READY) {
    game->state_timer += dt;
    // Gentle floating bob
    game->bird.y = BIRD_START_Y + 0.65f * sinf(game->state_timer * 4.5f);
    game->bird.vy = 0.0f;
    return;
  }

  if (game->state == STATE_PLAYING) {
    game->state_timer += dt;

    // Apply gravity
    game->bird.vy += GRAVITY * dt;
    if (game->bird.vy > MAX_FALL_SPEED) {
      game->bird.vy = MAX_FALL_SPEED;
    }
    if (game->bird.vy < MAX_RISE_SPEED) {
      game->bird.vy = MAX_RISE_SPEED;
    }
    game->bird.y += game->bird.vy * dt;

    // Ceiling clamp
    if (game->bird.y < 0.0f) {
      game->bird.y = 0.0f;
      if (game->bird.vy < 0.0f) {
        game->bird.vy = 0.0f;
      }
    }

    // Scroll pipes & check scoring
    for (int i = 0; i < MAX_PIPES; i++) {
      game->pipes[i].x -= PIPE_SPEED * dt;

      // Score increment when bird clears the pipe
      if (!game->pipes[i].passed && (game->pipes[i].x + (float)PIPE_WIDTH < BIRD_X)) {
        game->pipes[i].passed = true;
        game->score++;
        if (game->score > game->high_score) {
          game->high_score = game->score;
          game->is_new_high = true;
          game_save_high_score(game);
        }
        game->medal = game_get_medal(game->score);
      }

      // Recycle pipe when it scrolls off screen
      if (game->pipes[i].x + (float)PIPE_WIDTH < -2.0f) {
        float max_x = 0.0f;
        for (int p = 0; p < MAX_PIPES; p++) {
          if (game->pipes[p].x > max_x) {
            max_x = game->pipes[p].x;
          }
        }
        game->pipes[i].x = max_x + PIPE_SPACING;
        game->pipes[i].gap_y = 2 + (rand() % (SKY_HEIGHT - PIPE_GAP - 3));
        game->pipes[i].passed = false;
      }
    }

    // Scroll ground texture
    game->ground_offset += PIPE_SPEED * dt;
    if (game->ground_offset >= 8.0f) {
      game->ground_offset -= 8.0f;
    }

    // Check collisions
    if (game_check_collision(game)) {
      game->state = STATE_GAME_OVER;
      game->state_timer = 0.0f;
      game->flash_timer_ms = 350;
      game_delete_save();
    }
    return;
  }

  if (game->state == STATE_GAME_OVER) {
    game->state_timer += dt;
    // Bird falls to ground if in air
    if (game->bird.y < (float)(SKY_HEIGHT - 1)) {
      game->bird.vy += GRAVITY * dt * 1.5f;
      game->bird.y += game->bird.vy * dt;
      if (game->bird.y >= (float)(SKY_HEIGHT - 1)) {
        game->bird.y = (float)(SKY_HEIGHT - 1);
        game->bird.vy = 0.0f;
      }
    }
  }
}
