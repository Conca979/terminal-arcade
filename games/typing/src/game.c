#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "../../../include/scores.h"

#define SAVE_MAGIC_TYPING 0x54595045 // "TYPE"
#define SAVE_VERSION_TYPING 1

static char WORD_BANK[MAX_BANK_WORDS][MAX_WORD_LEN];
static int WORD_BANK_COUNT = 0;

static const char *FALLBACK_WORDS[] = {
  "laser", "shield", "vector", "matrix", "citadel", "missile", "defense",
  "system", "orbital", "cyber", "plasma", "cannon", "turret", "target",
  "secure", "protocol", "quantum", "command", "station", "warning",
  "breach", "reactor", "teleport", "overload", "gravity", "firewall",
  "network", "offline", "online", "destroy", "surface", "outpost",
  "carrier", "tactical", "kinetic", "barrier", "stellar", "warfare"
};
static const int FALLBACK_COUNT = sizeof(FALLBACK_WORDS) / sizeof(FALLBACK_WORDS[0]);

static const char *get_words_path(void) {
  FILE *f = fopen("games/typing/words.txt", "r");
  if (f) {
    fclose(f);
    return "games/typing/words.txt";
  }
  f = fopen("words.txt", "r");
  if (f) {
    fclose(f);
    return "words.txt";
  }
  f = fopen("../games/typing/words.txt", "r");
  if (f) {
    fclose(f);
    return "../games/typing/words.txt";
  }
  f = fopen("../../games/typing/words.txt", "r");
  if (f) {
    fclose(f);
    return "../../games/typing/words.txt";
  }
  return "games/typing/words.txt";
}

static void safe_word_copy(char *dest, const char *src, size_t dest_size) {
  if (!dest || dest_size == 0) return;
  size_t i = 0;
  while (i + 1 < dest_size && src[i] != '\0') {
    dest[i] = src[i];
    i++;
  }
  dest[i] = '\0';
}

int game_load_words(void) {
  if (WORD_BANK_COUNT > 0) {
    return WORD_BANK_COUNT;
  }

  const char *path = get_words_path();
  FILE *f = fopen(path, "r");
  if (!f) {
    for (int i = 0; i < FALLBACK_COUNT && i < MAX_BANK_WORDS; i++) {
      safe_word_copy(WORD_BANK[i], FALLBACK_WORDS[i], sizeof(WORD_BANK[i]));
      WORD_BANK_COUNT++;
    }
    return WORD_BANK_COUNT;
  }

  char line[64];
  while (fgets(line, sizeof(line), f) && WORD_BANK_COUNT < MAX_BANK_WORDS) {
    size_t len = strlen(line);
    while (len > 0 && (line[len - 1] == '\r' || line[len - 1] == '\n' || line[len - 1] == ' ')) {
      line[--len] = '\0';
    }

    if (len >= 3 && len <= 16) {
      bool valid = true;
      for (size_t i = 0; i < len; i++) {
        if (!isalpha(line[i])) {
          valid = false;
          break;
        }
        line[i] = (char)tolower(line[i]);
      }
      if (valid) {
        safe_word_copy(WORD_BANK[WORD_BANK_COUNT], line, sizeof(WORD_BANK[WORD_BANK_COUNT]));
        WORD_BANK_COUNT++;
      }
    }
  }

  fclose(f);

  if (WORD_BANK_COUNT == 0) {
    for (int i = 0; i < FALLBACK_COUNT && i < MAX_BANK_WORDS; i++) {
      safe_word_copy(WORD_BANK[i], FALLBACK_WORDS[i], sizeof(WORD_BANK[i]));
      WORD_BANK_COUNT++;
    }
  }

  return WORD_BANK_COUNT;
}

const char *game_get_random_word(int min_len, int max_len) {
  if (WORD_BANK_COUNT == 0) {
    game_load_words();
  }

  // Try up to 40 random picks for matching length criteria
  for (int attempt = 0; attempt < 40; attempt++) {
    int idx = rand() % WORD_BANK_COUNT;
    int len = (int)strlen(WORD_BANK[idx]);
    if (len >= min_len && len <= max_len) {
      return WORD_BANK[idx];
    }
  }

  // Fallback to random pick
  int idx = rand() % WORD_BANK_COUNT;
  return WORD_BANK[idx];
}

static const char *get_save_path(void) {
  FILE *f = fopen("games/typing/Makefile", "r");
  if (f) {
    fclose(f);
    return "games/typing/savegame.dat";
  }
  return "savegame.dat";
}

void game_load_high_score(GameState *game) {
  game->high_score = arcade_score_get("typing", "high_score");
  game->max_wpm = arcade_score_get("typing", "max_wpm");
}

void game_save_high_score(const GameState *game) {
  arcade_score_set("typing", "high_score", game->high_score);
  arcade_score_set("typing", "max_wpm", game->max_wpm);
}

void game_delete_save(void) {
  remove(get_save_path());
}

void game_save(const GameState *game) {
  if (game->state == STATE_GAME_OVER) return;

  FILE *f = fopen(get_save_path(), "wb");
  if (!f) return;

  uint32_t magic = SAVE_MAGIC_TYPING;
  uint32_t ver = SAVE_VERSION_TYPING;
  fwrite(&magic, sizeof(uint32_t), 1, f);
  fwrite(&ver, sizeof(uint32_t), 1, f);

  fwrite(&game->wave, sizeof(int), 1, f);
  fwrite(&game->words_spawned_in_wave, sizeof(int), 1, f);
  fwrite(&game->words_defeated_in_wave, sizeof(int), 1, f);
  fwrite(&game->shield_hp, sizeof(int), 1, f);
  fwrite(&game->score, sizeof(int), 1, f);
  fwrite(&game->words_completed, sizeof(int), 1, f);
  fwrite(&game->total_keystrokes, sizeof(int), 1, f);
  fwrite(&game->correct_keystrokes, sizeof(int), 1, f);
  fwrite(&game->current_combo, sizeof(int), 1, f);
  fwrite(&game->max_combo, sizeof(int), 1, f);
  fwrite(&game->elapsed_active_ms, sizeof(unsigned long long), 1, f);

  fwrite(&game->target_enemy_idx, sizeof(int), 1, f);
  fwrite(game->enemies, sizeof(Enemy), MAX_ACTIVE_ENEMIES, f);

  fclose(f);
}

bool game_load(GameState *game) {
  FILE *f = fopen(get_save_path(), "rb");
  if (!f) return false;

  uint32_t magic = 0, ver = 0;
  if (fread(&magic, sizeof(uint32_t), 1, f) != 1 || magic != SAVE_MAGIC_TYPING) {
    fclose(f);
    return false;
  }
  if (fread(&ver, sizeof(uint32_t), 1, f) != 1 || ver != SAVE_VERSION_TYPING) {
    fclose(f);
    return false;
  }

  if (fread(&game->wave, sizeof(int), 1, f) != 1 ||
      fread(&game->words_spawned_in_wave, sizeof(int), 1, f) != 1 ||
      fread(&game->words_defeated_in_wave, sizeof(int), 1, f) != 1 ||
      fread(&game->shield_hp, sizeof(int), 1, f) != 1 ||
      fread(&game->score, sizeof(int), 1, f) != 1 ||
      fread(&game->words_completed, sizeof(int), 1, f) != 1 ||
      fread(&game->total_keystrokes, sizeof(int), 1, f) != 1 ||
      fread(&game->correct_keystrokes, sizeof(int), 1, f) != 1 ||
      fread(&game->current_combo, sizeof(int), 1, f) != 1 ||
      fread(&game->max_combo, sizeof(int), 1, f) != 1 ||
      fread(&game->elapsed_active_ms, sizeof(unsigned long long), 1, f) != 1 ||
      fread(&game->target_enemy_idx, sizeof(int), 1, f) != 1 ||
      fread(game->enemies, sizeof(Enemy), MAX_ACTIVE_ENEMIES, f) != MAX_ACTIVE_ENEMIES) {
    fclose(f);
    return false;
  }

  fclose(f);

  if (game->shield_hp <= 0) {
    game_delete_save();
    return false;
  }

  game->state = STATE_PLAYING;
  game->spawn_timer_ms = 800;
  game->wave_intro_timer_ms = 0;
  game->emp_flash_ms = 0;
  game->shield_flash_ms = 0;
  game->start_time_ms = 0;
  memset(game->explosions, 0, sizeof(game->explosions));
  memset(game->lasers, 0, sizeof(game->lasers));

  return true;
}

static void spawn_laser(GameState *game, float target_x, float target_y) {
  for (int i = 0; i < MAX_LASERS; i++) {
    if (!game->lasers[i].active) {
      game->lasers[i].active = true;
      game->lasers[i].target_x = target_x;
      game->lasers[i].target_y = target_y;
      game->lasers[i].lifetime_ms = 180;
      return;
    }
  }
}

static void spawn_explosion(GameState *game, float x, float y) {
  for (int i = 0; i < MAX_EXPLOSIONS; i++) {
    if (!game->explosions[i].active) {
      game->explosions[i].active = true;
      game->explosions[i].x = x;
      game->explosions[i].y = y;
      game->explosions[i].frame = 0;
      return;
    }
  }
}

void game_reset_mission(GameState *game) {
  game_delete_save();
  game->wave = 1;
  game->words_spawned_in_wave = 0;
  game->words_defeated_in_wave = 0;
  game->wave_intro_timer_ms = 2000;
  game->spawn_timer_ms = 1200;

  game->shield_hp = MAX_SHIELD_HP;
  game->score = 0;
  game->words_completed = 0;
  game->total_keystrokes = 0;
  game->correct_keystrokes = 0;
  game->current_combo = 1;
  game->max_combo = 1;

  game->start_time_ms = 0;
  game->elapsed_active_ms = 0;
  game->emp_flash_ms = 0;
  game->shield_flash_ms = 0;
  game->target_enemy_idx = -1;

  memset(game->enemies, 0, sizeof(game->enemies));
  memset(game->explosions, 0, sizeof(game->explosions));
  memset(game->lasers, 0, sizeof(game->lasers));

  game->state = STATE_WAVE_INTRO;
}

void game_init(GameState *game) {
  memset(game, 0, sizeof(GameState));
  game_load_words();
  game_load_high_score(game);

  if (!game_load(game)) {
    game_reset_mission(game);
  }
}

int game_get_wpm(const GameState *game) {
  if (game->elapsed_active_ms < 2000) return 0;
  float minutes = (float)game->elapsed_active_ms / 60000.0f;
  int wpm = (int)((game->correct_keystrokes / 5.0f) / minutes);
  return (wpm > 0) ? wpm : 0;
}

int game_get_accuracy(const GameState *game) {
  if (game->total_keystrokes == 0) return 100;
  int acc = (int)(((float)game->correct_keystrokes / (float)game->total_keystrokes) * 100.0f);
  return (acc <= 100) ? acc : 100;
}

static void spawn_enemy(GameState *game) {
  int slot = -1;
  for (int i = 0; i < MAX_ACTIVE_ENEMIES; i++) {
    if (!game->enemies[i].active) {
      slot = i;
      break;
    }
  }
  if (slot == -1) return;

  Enemy *e = &game->enemies[slot];
  e->active = true;

  // Determine target length based on wave
  int min_l = 3 + (game->wave / 2);
  int max_l = 5 + (game->wave / 2);
  if (min_l > 8) min_l = 8;
  if (max_l > 14) max_l = 14;

  const char *word = game_get_random_word(min_l, max_l);
  strncpy(e->word, word, MAX_WORD_LEN - 1);
  e->word[MAX_WORD_LEN - 1] = '\0';
  e->word_len = (int)strlen(e->word);
  e->typed_len = 0;

  // Pick safe X coordinate within arena boundaries
  int max_x = ARENA_WIDTH - e->word_len - 6;
  if (max_x < 6) max_x = 6;
  e->x = (float)(6 + (rand() % (max_x - 6 + 1)));
  e->y = (float)PLAYABLE_TOP;

  // Determine enemy type
  int roll = rand() % 100;
  if (roll < 8) {
    e->type = ENEMY_EMP; // 8% chance EMP bomb
  } else if (roll < 22) {
    e->type = ENEMY_TANK; // 14% chance armored missile (repairs shield)
  } else if (roll < 42) {
    e->type = ENEMY_FAST; // 20% chance hypersonic missile
  } else {
    e->type = ENEMY_NORMAL;
  }

  // Speed scaling
  float speed_mult = 1.0f;
  if (e->type == ENEMY_FAST) speed_mult = 1.55f;
  else if (e->type == ENEMY_TANK) speed_mult = 0.75f;

  e->speed = (BASE_DESCENT_SPEED + (float)(game->wave - 1) * SPEED_INCREASE_PER_WAVE) * speed_mult;

  game->words_spawned_in_wave++;
}

void game_handle_action(GameState *game, GameAction action) {
  if (action.type == ACT_QUIT) {
    if (game->state != STATE_GAME_OVER) {
      game_save(game);
    }
    game->state = STATE_EXIT;
    return;
  }

  if (action.type == ACT_PAUSE) {
    if (game->state == STATE_PLAYING) {
      game->state = STATE_PAUSED;
    } else if (game->state == STATE_PAUSED) {
      game->state = STATE_PLAYING;
    }
    return;
  }

  if (action.type == ACT_RESTART) {
    if (game->state == STATE_GAME_OVER) {
      game_reset_mission(game);
    } else if (game->state == STATE_WAVE_INTRO) {
      game->wave_intro_timer_ms = 0; // Skip intro immediately
    }
    return;
  }

  if (game->state != STATE_PLAYING) {
    return;
  }

  if (action.type == ACT_BACKSPACE) {
    // Cancel active target lock
    if (game->target_enemy_idx != -1) {
      game->enemies[game->target_enemy_idx].typed_len = 0;
      game->target_enemy_idx = -1;
    }
    return;
  }

  if (action.type == ACT_CHAR) {
    char ch = action.ch;
    game->total_keystrokes++;

    if (game->target_enemy_idx == -1) {
      // Find lowest matching enemy starting with 'ch'
      int best_idx = -1;
      float max_y = -1.0f;

      for (int i = 0; i < MAX_ACTIVE_ENEMIES; i++) {
        if (game->enemies[i].active && game->enemies[i].word[0] == ch) {
          if (game->enemies[i].y > max_y) {
            max_y = game->enemies[i].y;
            best_idx = i;
          }
        }
      }

      if (best_idx != -1) {
        game->target_enemy_idx = best_idx;
        Enemy *target = &game->enemies[best_idx];
        target->typed_len = 1;
        game->correct_keystrokes++;
        game->score += 10 * game->current_combo;

        if (target->typed_len >= target->word_len) {
          // Word was 1 letter long
          spawn_laser(game, target->x + (float)target->word_len / 2.0f, target->y);
          spawn_explosion(game, target->x + (float)target->word_len / 2.0f, target->y);
          target->active = false;
          game->target_enemy_idx = -1;
          game->words_completed++;
          game->words_defeated_in_wave++;
          game->current_combo++;
          if (game->current_combo > game->max_combo) {
            game->max_combo = game->current_combo;
          }
        }
      } else {
        // Missed keystroke
        game->current_combo = 1;
      }
    } else {
      // Advance locked target
      Enemy *target = &game->enemies[game->target_enemy_idx];
      if (target->word[target->typed_len] == ch) {
        target->typed_len++;
        game->correct_keystrokes++;
        game->score += 10 * game->current_combo;

        if (target->typed_len >= target->word_len) {
          // Word fully typed! Destroy target!
          spawn_laser(game, target->x + (float)target->word_len / 2.0f, target->y);
          spawn_explosion(game, target->x + (float)target->word_len / 2.0f, target->y);

          int word_pts = target->word_len * 30 * game->current_combo;
          game->score += word_pts;
          game->words_completed++;
          game->words_defeated_in_wave++;

          game->current_combo++;
          if (game->current_combo > game->max_combo) {
            game->max_combo = game->current_combo;
          }

          if (target->type == ENEMY_TANK) {
            if (game->shield_hp < MAX_SHIELD_HP) {
              game->shield_hp++;
            }
          } else if (target->type == ENEMY_EMP) {
            game->emp_flash_ms = 400;
            for (int j = 0; j < MAX_ACTIVE_ENEMIES; j++) {
              if (game->enemies[j].active && j != game->target_enemy_idx) {
                spawn_explosion(game, game->enemies[j].x, game->enemies[j].y);
                game->enemies[j].active = false;
                game->score += 50 * game->current_combo;
                game->words_completed++;
                game->words_defeated_in_wave++;
              }
            }
          }

          target->active = false;
          game->target_enemy_idx = -1;

          if (game->score > game->high_score) {
            game->high_score = game->score;
            game_save_high_score(game);
          }

          int cur_wpm = game_get_wpm(game);
          if (cur_wpm > game->max_wpm) {
            game->max_wpm = cur_wpm;
            game_save_high_score(game);
          }
        }
      } else {
        // Mistake on locked target
        game->current_combo = 1;
      }
    }
  }
}

void game_update(GameState *game, float dt) {
  if (dt <= 0.0f || dt > 0.1f) dt = 0.016f;

  if (game->state == STATE_PAUSED || game->state == STATE_GAME_OVER || game->state == STATE_EXIT) {
    return;
  }

  // Update visual effects
  if (game->emp_flash_ms > 0) game->emp_flash_ms -= (int)(dt * 1000.0f);
  if (game->shield_flash_ms > 0) game->shield_flash_ms -= (int)(dt * 1000.0f);

  for (int i = 0; i < MAX_LASERS; i++) {
    if (game->lasers[i].active) {
      game->lasers[i].lifetime_ms -= (int)(dt * 1000.0f);
      if (game->lasers[i].lifetime_ms <= 0) {
        game->lasers[i].active = false;
      }
    }
  }

  for (int i = 0; i < MAX_EXPLOSIONS; i++) {
    if (game->explosions[i].active) {
      game->explosions[i].frame++;
      if (game->explosions[i].frame > 8) {
        game->explosions[i].active = false;
      }
    }
  }

  // Wave Intro countdown
  if (game->state == STATE_WAVE_INTRO) {
    game->wave_intro_timer_ms -= (int)(dt * 1000.0f);
    if (game->wave_intro_timer_ms <= 0) {
      game->state = STATE_PLAYING;
    }
    return;
  }

  // Accumulate active gameplay time for WPM
  game->elapsed_active_ms += (unsigned long long)(dt * 1000.0f);

  // Check wave victory
  if (game->words_defeated_in_wave >= WORDS_PER_WAVE) {
    game->wave++;
    game->words_spawned_in_wave = 0;
    game->words_defeated_in_wave = 0;
    game->target_enemy_idx = -1;
    game->state = STATE_WAVE_INTRO;
    game->wave_intro_timer_ms = 2200;
    game->spawn_timer_ms = 1200;
    game_save(game);
    return;
  }

  // Spawn new enemies
  game->spawn_timer_ms -= (int)(dt * 1000.0f);
  if (game->spawn_timer_ms <= 0 && game->words_spawned_in_wave < WORDS_PER_WAVE) {
    spawn_enemy(game);
    int interval = 2200 - (game->wave * 120);
    if (interval < 800) interval = 800;
    game->spawn_timer_ms = interval;
  }

  // Descend active enemies
  for (int i = 0; i < MAX_ACTIVE_ENEMIES; i++) {
    if (game->enemies[i].active) {
      game->enemies[i].y += game->enemies[i].speed * dt;

      // Check if enemy hit defense perimeter
      if (game->enemies[i].y >= (float)PLAYABLE_BOTTOM) {
        spawn_explosion(game, game->enemies[i].x, (float)PLAYABLE_BOTTOM);
        game->enemies[i].active = false;
        if (game->target_enemy_idx == i) {
          game->target_enemy_idx = -1;
        }

        game->shield_hp--;
        game->shield_flash_ms = 400;
        game->current_combo = 1;
        game->words_defeated_in_wave++;

        if (game->shield_hp <= 0) {
          game->state = STATE_GAME_OVER;
          game_delete_save();
          game_save_high_score(game);
          return;
        }
      }
    }
  }
}
