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

static void trigger_explosion(GameState *game, int x, int y, unsigned long long current_time) {
  for (int i = 0; i < MAX_EXPLOSIONS; i++) {
    if (!game->explosions[i].active) {
      game->explosions[i].active = true;
      game->explosions[i].x = x;
      game->explosions[i].y = y;
      game->explosions[i].expire_time = current_time + 220;
      break;
    }
  }
}

static void reset_bunkers(GameState *game) {
  int spacing = FIELD_WIDTH / (BUNKER_COUNT + 1);
  for (int i = 0; i < BUNKER_COUNT; i++) {
    game->bunkers[i].x = spacing * (i + 1) - (BUNKER_PARTS / 2);
    game->bunkers[i].y = 18;
    for (int p = 0; p < BUNKER_PARTS; p++) {
      game->bunkers[i].hp[p] = BUNKER_MAX_HP;
    }
  }
}

void game_next_wave(GameState *game) {
  game->wave++;
  game->swarm.origin_x = 4;
  game->swarm.origin_y = 3;
  game->swarm.dir_x = 1;
  game->swarm.alive_count = TOTAL_ALIENS;
  game->swarm.anim_frame = 0;
  game->swarm.last_move_time = 0;
  game->swarm.last_bomb_time = 0;

  for (int r = 0; r < ALIEN_ROWS; r++) {
    for (int c = 0; c < ALIEN_COLS; c++) {
      game->aliens[r][c].alive = true;
      if (r == 0) game->aliens[r][c].type = 0;      // Squid (top)
      else if (r == 3) game->aliens[r][c].type = 2; // Octopus (bottom)
      else game->aliens[r][c].type = 1;             // Crab (middle)
    }
  }

  // Clear bullets
  memset(game->player_bullets, 0, sizeof(game->player_bullets));
  memset(game->alien_bombs, 0, sizeof(game->alien_bombs));
  memset(game->explosions, 0, sizeof(game->explosions));

  game->ufo.active = false;
  game->next_ufo_time = 18000 + (rand() % 10000);

  reset_bunkers(game);
  game->state = STATE_PLAYING;
}

void game_reset(GameState *game) {
  game->player.x = FIELD_WIDTH / 2;
  game->player.lives = PLAYER_START_LIVES;
  game->player.last_fire_time = 0;

  game->score = 0;
  game->wave = 0;
  game->state = STATE_PLAYING;

  game_next_wave(game);
}

void game_init(GameState *game) {
  game_load_high_score(game);
  game_reset(game);
}

void game_fire_bullet(GameState *game, unsigned long long current_time) {
  if (game->state != STATE_PLAYING) {
    return;
  }
  // Cooldown between shots: 260ms
  if (game->player.last_fire_time > 0 && current_time - game->player.last_fire_time < 260) {
    return;
  }

  for (int i = 0; i < MAX_PLAYER_BULLETS; i++) {
    if (!game->player_bullets[i].active) {
      game->player_bullets[i].active = true;
      game->player_bullets[i].x = game->player.x;
      game->player_bullets[i].y = 21;
      game->player_bullets[i].dy = -1;
      game->player.last_fire_time = current_time;
      break;
    }
  }
}

void game_handle_action(GameState *game, GameAction action, unsigned long long current_time) {
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

  if (game->state != STATE_PLAYING) {
    return;
  }

  switch (action) {
    case ACT_LEFT:
      if (game->player.x > 2) {
        game->player.x -= 2;
      }
      break;

    case ACT_RIGHT:
      if (game->player.x < FIELD_WIDTH - 3) {
        game->player.x += 2;
      }
      break;

    case ACT_FIRE:
      game_fire_bullet(game, current_time);
      break;

    default:
      break;
  }
}

static void update_swarm_movement(GameState *game, unsigned long long current_time) {
  // Speed scales with remaining aliens and wave level
  int base_ms = 80 + (game->swarm.alive_count * 18) - (game->wave * 12);
  if (base_ms < 60) base_ms = 60;

  if (current_time - game->swarm.last_move_time < (unsigned long long)base_ms) {
    return;
  }
  game->swarm.last_move_time = current_time;
  game->swarm.anim_frame = 1 - game->swarm.anim_frame;

  // Find left-most and right-most alive aliens
  int min_c = ALIEN_COLS;
  int max_c = -1;
  int max_r = -1;

  for (int r = 0; r < ALIEN_ROWS; r++) {
    for (int c = 0; c < ALIEN_COLS; c++) {
      if (game->aliens[r][c].alive) {
        if (c < min_c) min_c = c;
        if (c > max_c) max_c = c;
        if (r > max_r) max_r = r;
      }
    }
  }

  if (game->swarm.alive_count == 0) {
    return;
  }

  int left_edge_x = game->swarm.origin_x + min_c * 4;
  int right_edge_x = game->swarm.origin_x + max_c * 4 + 2;
  int lowest_y = game->swarm.origin_y + max_r * 2;

  // If aliens invaded player position -> Instant Loss
  if (lowest_y >= 21) {
    game->state = STATE_GAME_OVER;
    return;
  }

  // Check border bounce and shift down
  if (game->swarm.dir_x > 0 && right_edge_x >= FIELD_WIDTH - 2) {
    game->swarm.dir_x = -1;
    game->swarm.origin_y++;
  } else if (game->swarm.dir_x < 0 && left_edge_x <= 2) {
    game->swarm.dir_x = 1;
    game->swarm.origin_y++;
  } else {
    game->swarm.origin_x += game->swarm.dir_x;
  }
}

static void update_alien_bombs(GameState *game, unsigned long long current_time) {
  // Alien bomb drop cadence: ~700-1100ms
  int bomb_interval = 700 + (rand() % 400);
  if (current_time - game->swarm.last_bomb_time >= (unsigned long long)bomb_interval && game->swarm.alive_count > 0) {
    game->swarm.last_bomb_time = current_time;

    // Pick random column that has alive aliens
    int valid_cols[ALIEN_COLS];
    int valid_count = 0;
    for (int c = 0; c < ALIEN_COLS; c++) {
      for (int r = ALIEN_ROWS - 1; r >= 0; r--) {
        if (game->aliens[r][c].alive) {
          valid_cols[valid_count++] = c;
          break;
        }
      }
    }

    if (valid_count > 0) {
      int col = valid_cols[rand() % valid_count];
      // Find lowest alien in that column
      int low_r = -1;
      for (int r = ALIEN_ROWS - 1; r >= 0; r--) {
        if (game->aliens[r][col].alive) {
          low_r = r;
          break;
        }
      }

      if (low_r >= 0) {
        int bomb_x = game->swarm.origin_x + col * 4 + 1;
        int bomb_y = game->swarm.origin_y + low_r * 2 + 1;

        for (int i = 0; i < MAX_ALIEN_BOMBS; i++) {
          if (!game->alien_bombs[i].active) {
            game->alien_bombs[i].active = true;
            game->alien_bombs[i].x = bomb_x;
            game->alien_bombs[i].y = bomb_y;
            game->alien_bombs[i].dy = 1;
            break;
          }
        }
      }
    }
  }

  // Move bombs downward
  static unsigned long long last_bomb_step = 0;
  if (current_time - last_bomb_step >= 60) {
    last_bomb_step = current_time;
    for (int i = 0; i < MAX_ALIEN_BOMBS; i++) {
      if (game->alien_bombs[i].active) {
        game->alien_bombs[i].y++;
        if (game->alien_bombs[i].y >= FIELD_HEIGHT) {
          game->alien_bombs[i].active = false;
        }
      }
    }
  }
}

static void update_player_bullets(GameState *game, unsigned long long current_time) {
  static unsigned long long last_bullet_step = 0;
  if (current_time - last_bullet_step >= 35) {
    last_bullet_step = current_time;
    for (int i = 0; i < MAX_PLAYER_BULLETS; i++) {
      if (game->player_bullets[i].active) {
        game->player_bullets[i].y--;
        if (game->player_bullets[i].y <= 0) {
          game->player_bullets[i].active = false;
        }
      }
    }
  }
}

static void update_ufo(GameState *game, unsigned long long current_time) {
  if (!game->ufo.active && current_time >= game->next_ufo_time) {
    game->ufo.active = true;
    game->ufo.y = 1;
    if (rand() % 2 == 0) {
      game->ufo.x = 1;
      game->ufo.dir_x = 1;
    } else {
      game->ufo.x = FIELD_WIDTH - 4;
      game->ufo.dir_x = -1;
    }
    int scores[4] = {100, 150, 200, 300};
    game->ufo.score_val = scores[rand() % 4];
    game->next_ufo_time = current_time + UFO_SPAWN_INTERVAL_MS + (rand() % 10000);
  }

  static unsigned long long last_ufo_step = 0;
  if (game->ufo.active && current_time - last_ufo_step >= 80) {
    last_ufo_step = current_time;
    game->ufo.x += game->ufo.dir_x;
    if (game->ufo.x < 1 || game->ufo.x >= FIELD_WIDTH - 3) {
      game->ufo.active = false;
    }
  }
}

static void check_collisions(GameState *game, unsigned long long current_time) {
  // 1. Player Bullets vs Aliens
  for (int b = 0; b < MAX_PLAYER_BULLETS; b++) {
    if (!game->player_bullets[b].active) continue;
    int bx = game->player_bullets[b].x;
    int by = game->player_bullets[b].y;

    // Check UFO collision
    if (game->ufo.active && by == game->ufo.y && (bx >= game->ufo.x && bx <= game->ufo.x + 3)) {
      game->score += game->ufo.score_val;
      game->ufo.active = false;
      game->player_bullets[b].active = false;
      trigger_explosion(game, bx, by, current_time);
      continue;
    }

    // Check Aliens
    for (int r = 0; r < ALIEN_ROWS; r++) {
      for (int c = 0; c < ALIEN_COLS; c++) {
        if (!game->aliens[r][c].alive) continue;
        int ax = game->swarm.origin_x + c * 4;
        int ay = game->swarm.origin_y + r * 2;

        if ((by == ay || by == ay + 1) && (bx >= ax && bx <= ax + 2)) {
          game->aliens[r][c].alive = false;
          game->swarm.alive_count--;
          game->player_bullets[b].active = false;

          int pts = (game->aliens[r][c].type == 0) ? 30 :
                    (game->aliens[r][c].type == 1) ? 20 : 10;
          game->score += pts;
          trigger_explosion(game, ax + 1, ay, current_time);
          break;
        }
      }
      if (!game->player_bullets[b].active) break;
    }

    if (!game->player_bullets[b].active) continue;

    // 2. Player Bullets vs Bunkers
    for (int k = 0; k < BUNKER_COUNT; k++) {
      if (by == game->bunkers[k].y) {
        int part = bx - game->bunkers[k].x;
        if (part >= 0 && part < BUNKER_PARTS && game->bunkers[k].hp[part] > 0) {
          game->bunkers[k].hp[part]--;
          game->player_bullets[b].active = false;
          break;
        }
      }
    }
  }

  // 3. Alien Bombs vs Bunkers & Player
  for (int b = 0; b < MAX_ALIEN_BOMBS; b++) {
    if (!game->alien_bombs[b].active) continue;
    int bx = game->alien_bombs[b].x;
    int by = game->alien_bombs[b].y;

    // Check vs Bunkers
    for (int k = 0; k < BUNKER_COUNT; k++) {
      if (by == game->bunkers[k].y) {
        int part = bx - game->bunkers[k].x;
        if (part >= 0 && part < BUNKER_PARTS && game->bunkers[k].hp[part] > 0) {
          game->bunkers[k].hp[part]--;
          game->alien_bombs[b].active = false;
          break;
        }
      }
    }

    if (!game->alien_bombs[b].active) continue;

    // Check vs Player Cannon (at row 22, width 3)
    if (by >= 22 && abs(bx - game->player.x) <= 1) {
      game->alien_bombs[b].active = false;
      game->player.lives--;
      trigger_explosion(game, game->player.x, 22, current_time);

      if (game->player.lives <= 0) {
        game->state = STATE_GAME_OVER;
      }
      break;
    }

    // 4. Player bullet vs Alien bomb collision
    for (int pb = 0; pb < MAX_PLAYER_BULLETS; pb++) {
      if (game->player_bullets[pb].active &&
          abs(game->player_bullets[pb].x - bx) <= 1 &&
          abs(game->player_bullets[pb].y - by) <= 1) {
        game->player_bullets[pb].active = false;
        game->alien_bombs[b].active = false;
        trigger_explosion(game, bx, by, current_time);
        break;
      }
    }
  }

  // Check explosions expiration
  for (int i = 0; i < MAX_EXPLOSIONS; i++) {
    if (game->explosions[i].active && current_time >= game->explosions[i].expire_time) {
      game->explosions[i].active = false;
    }
  }

  // Update high score
  if (game->score > game->high_score) {
    game->high_score = game->score;
    game_save_high_score(game);
  }

  // Check Wave Clear
  if (game->swarm.alive_count == 0 && game->state == STATE_PLAYING) {
    game->state = STATE_WAVE_CLEAR;
    game->wave_clear_time = current_time;
  }
}

void game_update(GameState *game, unsigned long long current_time) {
  if (game->state == STATE_WAVE_CLEAR) {
    if (current_time - game->wave_clear_time >= 1500) {
      game_next_wave(game);
    }
    return;
  }

  if (game->state != STATE_PLAYING) {
    return;
  }

  update_swarm_movement(game, current_time);
  update_alien_bombs(game, current_time);
  update_player_bullets(game, current_time);
  update_ufo(game, current_time);
  check_collisions(game, current_time);
}
