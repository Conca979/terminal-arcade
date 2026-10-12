#include "game.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../../include/scores.h"

#define SAVE_MAGIC_PONG 0x504F4E47 // "PONG"
#define SAVE_VERSION_PONG 1

static const char *get_save_path(void) {
  FILE *f = fopen("games/pong/Makefile", "r");
  if (f) {
    fclose(f);
    return "games/pong/savegame.dat";
  }
  return "savegame.dat";
}

void game_load_high_score(GameState *game) {
  game->high_score = arcade_score_get("pong", "high_score");
}

void game_save_high_score(const GameState *game) {
  arcade_score_set("pong", "high_score", game->high_score);
}

void game_delete_save(void) {
  remove(get_save_path());
}

void game_save(const GameState *game) {
  if (game->state == STATE_GAME_OVER || game->state == STATE_EXIT) return;

  FILE *f = fopen(get_save_path(), "wb");
  if (!f) return;

  uint32_t magic = SAVE_MAGIC_PONG;
  uint32_t ver = SAVE_VERSION_PONG;
  fwrite(&magic, sizeof(uint32_t), 1, f);
  fwrite(&ver, sizeof(uint32_t), 1, f);

  fwrite(&game->player_score, sizeof(int), 1, f);
  fwrite(&game->cpu_score, sizeof(int), 1, f);
  fwrite(&game->rally_count, sizeof(int), 1, f);
  fwrite(&game->max_rally, sizeof(int), 1, f);
  int diff = (int)game->difficulty;
  fwrite(&diff, sizeof(int), 1, f);
  int st = (int)STATE_SERVE; // Always persist resumable state as STATE_SERVE
  fwrite(&st, sizeof(int), 1, f);

  fwrite(&game->player.y, sizeof(float), 1, f);
  fwrite(&game->cpu.y, sizeof(float), 1, f);

  fwrite(&game->ball.x, sizeof(float), 1, f);
  fwrite(&game->ball.y, sizeof(float), 1, f);
  fwrite(&game->ball.vx, sizeof(float), 1, f);
  fwrite(&game->ball.vy, sizeof(float), 1, f);

  fclose(f);
}

bool game_load(GameState *game) {
  FILE *f = fopen(get_save_path(), "rb");
  if (!f) return false;

  uint32_t magic = 0;
  uint32_t ver = 0;
  if (fread(&magic, sizeof(uint32_t), 1, f) != 1 || magic != SAVE_MAGIC_PONG) {
    fclose(f);
    return false;
  }
  if (fread(&ver, sizeof(uint32_t), 1, f) != 1 || ver != SAVE_VERSION_PONG) {
    fclose(f);
    return false;
  }

  if (fread(&game->player_score, sizeof(int), 1, f) != 1 ||
      fread(&game->cpu_score, sizeof(int), 1, f) != 1 ||
      fread(&game->rally_count, sizeof(int), 1, f) != 1 ||
      fread(&game->max_rally, sizeof(int), 1, f) != 1) {
    fclose(f);
    return false;
  }

  int diff = 0, st = 0;
  if (fread(&diff, sizeof(int), 1, f) != 1 ||
      fread(&st, sizeof(int), 1, f) != 1) {
    fclose(f);
    return false;
  }
  (void)st;
  game->difficulty = (AiDifficulty)diff;
  game->state = STATE_SERVE; // Always resume ready to serve

  if (fread(&game->player.y, sizeof(float), 1, f) != 1 ||
      fread(&game->cpu.y, sizeof(float), 1, f) != 1 ||
      fread(&game->ball.x, sizeof(float), 1, f) != 1 ||
      fread(&game->ball.y, sizeof(float), 1, f) != 1 ||
      fread(&game->ball.vx, sizeof(float), 1, f) != 1 ||
      fread(&game->ball.vy, sizeof(float), 1, f) != 1) {
    fclose(f);
    return false;
  }

  fclose(f);

  game->ball.trail_count = 0;
  game->player.vy = 0.0f;
  game->cpu.vy = 0.0f;
  game->player.hit_flash = 0;
  game->cpu.hit_flash = 0;
  game->serve_timer_ms = 0;
  game->scorer = 0;

  if (game->player_score >= WIN_SCORE || game->cpu_score >= WIN_SCORE) {
    game_delete_save();
    return false;
  }

  return true;
}

void game_serve_ball(GameState *game, int toward_dir) {
  game->ball.x = (float)(COURT_WIDTH / 2);
  game->ball.y = (float)(COURT_HEIGHT / 2);

  float dir = (toward_dir >= 0) ? 1.0f : -1.0f;
  game->ball.vx = dir * INITIAL_BALL_SPEED_X;

  float rand_factor = (((float)rand() / (float)RAND_MAX) * 2.0f) - 1.0f;
  game->ball.vy = rand_factor * 9.0f;

  game->ball.trail_count = 0;
  game->state = STATE_PLAYING;
  game->serve_timer_ms = 0;
}

void game_reset_match(GameState *game) {
  game_delete_save();
  game->player_score = 0;
  game->cpu_score = 0;
  game->rally_count = 0;
  game->max_rally = 0;
  game->scorer = 0;

  game->player.y = (float)(COURT_HEIGHT / 2);
  game->player.vy = 0.0f;
  game->player.hit_flash = 0;

  game->cpu.y = (float)(COURT_HEIGHT / 2);
  game->cpu.vy = 0.0f;
  game->cpu.hit_flash = 0;

  game->state = STATE_SERVE;
  game->ball.x = (float)(COURT_WIDTH / 2);
  game->ball.y = (float)(COURT_HEIGHT / 2);
  game->ball.vx = 0.0f;
  game->ball.vy = 0.0f;
  game->ball.trail_count = 0;
  game->serve_timer_ms = 0;
}

void game_init(GameState *game) {
  memset(game, 0, sizeof(GameState));
  game_load_high_score(game);
  game->difficulty = DIFF_MEDIUM;

  if (!game_load(game)) {
    game_reset_match(game);
  }
}

void game_handle_action(GameState *game, GameAction action) {
  switch (action) {
    case ACT_UP:
      game->player.y -= 1.8f;
      if (game->player.y < 3.0f) {
        game->player.y = 3.0f;
      }
      break;

    case ACT_DOWN:
      game->player.y += 1.8f;
      if (game->player.y > (float)(COURT_HEIGHT - 4)) {
        game->player.y = (float)(COURT_HEIGHT - 4);
      }
      break;

    case ACT_SERVE:
      if (game->state == STATE_SERVE || game->state == STATE_POINT_SCORED) {
        int dir = (game->scorer == 2) ? -1 : 1;
        game_serve_ball(game, dir);
      } else if (game->state == STATE_PAUSED) {
        game->state = STATE_PLAYING;
      } else if (game->state == STATE_GAME_OVER) {
        game_reset_match(game);
        game_serve_ball(game, 1);
      }
      break;

    case ACT_TOGGLE_DIFF:
      game->difficulty = (AiDifficulty)((game->difficulty + 1) % DIFF_COUNT);
      game_save(game);
      break;

    case ACT_PAUSE:
      if (game->state == STATE_PLAYING) {
        game->state = STATE_PAUSED;
      } else if (game->state == STATE_PAUSED) {
        game->state = STATE_PLAYING;
      }
      break;

    case ACT_RESTART:
      game_reset_match(game);
      game_serve_ball(game, 1);
      break;

    case ACT_QUIT:
      if (game->state != STATE_GAME_OVER && game->state != STATE_EXIT) {
        game_save(game);
      }
      game->state = STATE_EXIT;
      break;

    default:
      break;
  }
}

void game_update(GameState *game, float dt) {
  if (dt <= 0.0f || dt > 0.1f) {
    dt = 0.016f; // Clamp delta time to avoid tunnel physics
  }

  if (game->state == STATE_PAUSED || game->state == STATE_GAME_OVER || game->state == STATE_EXIT) {
    return;
  }

  // Update paddle hit flashes
  if (game->player.hit_flash > 0) game->player.hit_flash--;
  if (game->cpu.hit_flash > 0) game->cpu.hit_flash--;

  // Player paddle movement from continuous velocity
  if (game->player.vy != 0.0f) {
    game->player.y += game->player.vy * dt;
    if (game->player.y < 3.0f) {
      game->player.y = 3.0f;
    }
    if (game->player.y > (float)(COURT_HEIGHT - 4)) {
      game->player.y = (float)(COURT_HEIGHT - 4);
    }
  }

  // Handle Point Scored countdown
  if (game->state == STATE_POINT_SCORED) {
    game->serve_timer_ms -= (int)(dt * 1000.0f);
    if (game->serve_timer_ms <= 0) {
      int dir = (game->scorer == 2) ? -1 : 1;
      game_serve_ball(game, dir);
    }
    return;
  }

  // Handle Serve state (auto-serve after 2.5s if player has not pressed Space/Enter)
  if (game->state == STATE_SERVE) {
    game->serve_timer_ms += (int)(dt * 1000.0f);
    if (game->serve_timer_ms >= 2500) {
      int dir = (game->scorer == 2) ? -1 : 1;
      game_serve_ball(game, dir);
    }
    return;
  }

  // Record ball trail
  for (int i = TRAIL_MAX - 1; i > 0; i--) {
    game->ball.trail[i] = game->ball.trail[i - 1];
  }
  game->ball.trail[0].x = game->ball.x;
  game->ball.trail[0].y = game->ball.y;
  if (game->ball.trail_count < TRAIL_MAX) {
    game->ball.trail_count++;
  }

  // Advance ball
  game->ball.x += game->ball.vx * dt;
  game->ball.y += game->ball.vy * dt;

  // Bounce off top wall
  if (game->ball.y <= 1.0f && game->ball.vy < 0.0f) {
    game->ball.y = 1.0f;
    game->ball.vy = -game->ball.vy;
  }

  // Bounce off bottom wall
  if (game->ball.y >= (float)(COURT_HEIGHT - 2) && game->ball.vy > 0.0f) {
    game->ball.y = (float)(COURT_HEIGHT - 2);
    game->ball.vy = -game->ball.vy;
  }

  // Player Paddle Collision (x = 2)
  if (game->ball.vx < 0.0f && game->ball.x <= 3.2f && game->ball.x >= 1.5f) {
    float top = game->player.y - PADDLE_HALF - 0.7f;
    float bottom = game->player.y + PADDLE_HALF + 0.7f;
    if (game->ball.y >= top && game->ball.y <= bottom) {
      game->ball.x = 3.3f;
      game->ball.vx = -game->ball.vx * SPEED_UP_FACTOR;
      if (game->ball.vx > MAX_BALL_SPEED_X) {
        game->ball.vx = MAX_BALL_SPEED_X;
      }

      float offset = (game->ball.y - game->player.y) / PADDLE_HALF;
      if (offset < -1.0f) offset = -1.0f;
      if (offset > 1.0f) offset = 1.0f;
      game->ball.vy = offset * (fabsf(game->ball.vx) * 0.75f);

      game->player.hit_flash = 4;
      game->rally_count++;
      if (game->rally_count > game->max_rally) {
        game->max_rally = game->rally_count;
      }
      if (game->max_rally > game->high_score) {
        game->high_score = game->max_rally;
        game_save_high_score(game);
      }
    }
  }

  // CPU Paddle Collision (x = COURT_WIDTH - 3 = 73)
  float cpu_x = (float)(COURT_WIDTH - 3);
  if (game->ball.vx > 0.0f && game->ball.x >= cpu_x - 1.2f && game->ball.x <= cpu_x + 0.5f) {
    float top = game->cpu.y - PADDLE_HALF - 0.7f;
    float bottom = game->cpu.y + PADDLE_HALF + 0.7f;
    if (game->ball.y >= top && game->ball.y <= bottom) {
      game->ball.x = cpu_x - 1.3f;
      game->ball.vx = -game->ball.vx * SPEED_UP_FACTOR;
      if (-game->ball.vx > MAX_BALL_SPEED_X) {
        game->ball.vx = -MAX_BALL_SPEED_X;
      }

      float offset = (game->ball.y - game->cpu.y) / PADDLE_HALF;
      if (offset < -1.0f) offset = -1.0f;
      if (offset > 1.0f) offset = 1.0f;
      game->ball.vy = offset * (fabsf(game->ball.vx) * 0.75f);

      game->cpu.hit_flash = 4;
      game->rally_count++;
      if (game->rally_count > game->max_rally) {
        game->max_rally = game->rally_count;
      }
      if (game->max_rally > game->high_score) {
        game->high_score = game->max_rally;
        game_save_high_score(game);
      }
    }
  }

  // CPU AI Tracking
  float cpu_speed = 18.0f;
  if (game->difficulty == DIFF_EASY) {
    cpu_speed = 13.0f;
  } else if (game->difficulty == DIFF_HARD) {
    cpu_speed = 25.0f;
  }

  if (game->ball.vx > 0.0f) {
    float target_y = game->ball.y;
    float diff = target_y - game->cpu.y;
    if (fabsf(diff) > 0.6f) {
      float step = (diff > 0 ? 1.0f : -1.0f) * cpu_speed * dt;
      if (fabsf(step) > fabsf(diff)) {
        step = diff;
      }
      game->cpu.y += step;
    }
  } else {
    // Softly drift towards court center when ball is traveling away
    float center_y = (float)(COURT_HEIGHT / 2);
    float diff = center_y - game->cpu.y;
    if (fabsf(diff) > 1.0f) {
      game->cpu.y += (diff > 0 ? 1.0f : -1.0f) * 8.0f * dt;
    }
  }

  // Clamp CPU paddle position
  if (game->cpu.y < 3.0f) {
    game->cpu.y = 3.0f;
  }
  if (game->cpu.y > (float)(COURT_HEIGHT - 4)) {
    game->cpu.y = (float)(COURT_HEIGHT - 4);
  }

  // Goal Detection
  if (game->ball.x < 0.6f) {
    // CPU Scores
    game->cpu_score++;
    game->rally_count = 0;
    game->scorer = 2;
    if (game->cpu_score >= WIN_SCORE) {
      game->state = STATE_GAME_OVER;
      game_delete_save();
    } else {
      game->state = STATE_POINT_SCORED;
      game->serve_timer_ms = 1100;
      game_save(game);
    }
  } else if (game->ball.x > (float)(COURT_WIDTH - 1.6f)) {
    // Player Scores
    game->player_score++;
    game->rally_count = 0;
    game->scorer = 1;
    if (game->player_score >= WIN_SCORE) {
      game->state = STATE_GAME_OVER;
      int won = arcade_score_get("pong", "matches_won");
      arcade_score_set("pong", "matches_won", won + 1);
      game_delete_save();
    } else {
      game->state = STATE_POINT_SCORED;
      game->serve_timer_ms = 1100;
      game_save(game);
    }
  }
}
