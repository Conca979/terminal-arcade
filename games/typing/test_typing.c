#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "game.h"

static void test_initialization_and_word_bank(void) {
  game_delete_save();
  GameState game;
  game_init(&game);

  int word_count = game_load_words();
  assert(word_count >= 30); // Loaded either words.txt or fallback bank

  const char *sample = game_get_random_word(4, 6);
  assert(sample != NULL);
  int len = (int)strlen(sample);
  assert(len >= 4 && len <= 6);

  assert(game.shield_hp == MAX_SHIELD_HP);
  assert(game.score == 0);
  assert(game.wave == 1);
  assert(game.target_enemy_idx == -1);
  printf("[PASS] test_initialization_and_word_bank (words loaded: %d)\n", word_count);
}

static void test_targeting_and_typing_mechanic(void) {
  GameState game;
  game_init(&game);
  game.state = STATE_PLAYING;

  // Setup one active enemy
  game.enemies[0].active = true;
  strcpy(game.enemies[0].word, "laser");
  game.enemies[0].word_len = 5;
  game.enemies[0].typed_len = 0;
  game.enemies[0].x = 20.0f;
  game.enemies[0].y = 8.0f;
  game.enemies[0].type = ENEMY_NORMAL;

  // 1. Type 'l' -> locks target
  GameAction act;
  act.type = ACT_CHAR;
  act.ch = 'l';
  game_handle_action(&game, act);

  assert(game.target_enemy_idx == 0);
  assert(game.enemies[0].typed_len == 1);
  assert(game.correct_keystrokes == 1);

  // 2. Type 'a', 's', 'e', 'r' -> completes word
  const char *rest = "aser";
  for (int i = 0; i < 4; i++) {
    act.ch = rest[i];
    game_handle_action(&game, act);
  }

  // Enemy should be defeated
  assert(!game.enemies[0].active);
  assert(game.target_enemy_idx == -1);
  assert(game.words_completed == 1);
  assert(game.score > 0);
  assert(game.current_combo == 2);
  printf("[PASS] test_targeting_and_typing_mechanic\n");
}

static void test_mistake_and_combo_reset(void) {
  GameState game;
  game_init(&game);
  game.state = STATE_PLAYING;

  game.enemies[0].active = true;
  strcpy(game.enemies[0].word, "shield");
  game.enemies[0].word_len = 6;
  game.enemies[0].typed_len = 0;
  game.enemies[0].x = 15.0f;
  game.enemies[0].y = 10.0f;

  game.current_combo = 4;

  // Type correct 's'
  GameAction act;
  act.type = ACT_CHAR;
  act.ch = 's';
  game_handle_action(&game, act);
  assert(game.enemies[0].typed_len == 1);

  // Type incorrect letter 'z'
  act.ch = 'z';
  game_handle_action(&game, act);
  assert(game.current_combo == 1); // Combo reset
  assert(game.enemies[0].typed_len == 1); // Progress preserved
  printf("[PASS] test_mistake_and_combo_reset\n");
}

static void test_backspace_unlock(void) {
  GameState game;
  game_init(&game);
  game.state = STATE_PLAYING;

  game.enemies[0].active = true;
  strcpy(game.enemies[0].word, "missile");
  game.enemies[0].word_len = 7;
  game.enemies[0].typed_len = 0;

  // Lock target
  GameAction act;
  act.type = ACT_CHAR;
  act.ch = 'm';
  game_handle_action(&game, act);
  assert(game.target_enemy_idx == 0);

  // Press backspace
  act.type = ACT_BACKSPACE;
  act.ch = '\0';
  game_handle_action(&game, act);

  assert(game.target_enemy_idx == -1);
  assert(game.enemies[0].typed_len == 0);
  printf("[PASS] test_backspace_unlock\n");
}

static void test_enemy_descent_and_damage(void) {
  GameState game;
  game_init(&game);
  game.state = STATE_PLAYING;

  // Place enemy right above ground
  game.enemies[0].active = true;
  strcpy(game.enemies[0].word, "breach");
  game.enemies[0].word_len = 6;
  game.enemies[0].y = (float)PLAYABLE_BOTTOM - 0.2f;
  game.enemies[0].speed = 10.0f;

  int initial_hp = game.shield_hp;

  // Advance time
  game_update(&game, 0.05f);

  // Enemy should have hit citadel base and dealt damage
  assert(!game.enemies[0].active);
  assert(game.shield_hp == initial_hp - 1);
  printf("[PASS] test_enemy_descent_and_damage\n");
}

static void test_wpm_and_accuracy(void) {
  GameState game;
  game_init(&game);

  game.elapsed_active_ms = 60000; // 1 minute
  game.correct_keystrokes = 250; // 250 chars / 5 = 50 WPM
  game.total_keystrokes = 260;

  int wpm = game_get_wpm(&game);
  int acc = game_get_accuracy(&game);

  assert(wpm == 50);
  assert(acc == 96);
  printf("[PASS] test_wpm_and_accuracy (WPM: %d, Acc: %d%%)\n", wpm, acc);
}

static void test_auto_save_and_resume(void) {
  game_delete_save();
  GameState g1;
  game_init(&g1);
  g1.state = STATE_PLAYING;

  g1.wave = 3;
  g1.score = 2400;
  g1.shield_hp = 3;
  g1.current_combo = 5;
  g1.max_combo = 8;
  g1.words_completed = 14;

  g1.enemies[0].active = true;
  strcpy(g1.enemies[0].word, "citadel");
  g1.enemies[0].word_len = 7;
  g1.enemies[0].typed_len = 3;
  g1.enemies[0].x = 32.0f;
  g1.enemies[0].y = 12.5f;
  g1.enemies[0].speed = 1.4f;
  g1.target_enemy_idx = 0;

  game_save(&g1);

  // Resume in new instance
  GameState g2;
  game_init(&g2);

  assert(g2.wave == 3);
  assert(g2.score == 2400);
  assert(g2.shield_hp == 3);
  assert(g2.current_combo == 5);
  assert(g2.max_combo == 8);
  assert(g2.target_enemy_idx == 0);
  assert(g2.enemies[0].active);
  assert(strcmp(g2.enemies[0].word, "citadel") == 0);
  assert(g2.enemies[0].typed_len == 3);

  game_delete_save();
  printf("[PASS] test_auto_save_and_resume\n");
}

int main(void) {
  srand(42);
  printf("Starting Typing Defense Logic Tests...\n");
  test_initialization_and_word_bank();
  test_targeting_and_typing_mechanic();
  test_mistake_and_combo_reset();
  test_backspace_unlock();
  test_enemy_descent_and_damage();
  test_wpm_and_accuracy();
  test_auto_save_and_resume();
  printf("All Typing Defense tests PASSED successfully!\n");
  return 0;
}
