#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "game.h"

static void test_initialization(void) {
  game_delete_save();
  GameState game;
  game_init(&game);

  assert(game.player_score == 0);
  assert(game.cpu_score == 0);
  assert(game.rally_count == 0);
  assert(game.state == STATE_SERVE);
  assert(fabsf(game.player.y - (float)(COURT_HEIGHT / 2)) < 0.1f);
  assert(fabsf(game.cpu.y - (float)(COURT_HEIGHT / 2)) < 0.1f);
  printf("[PASS] test_initialization\n");
}

static void test_paddle_movement_and_clamping(void) {
  GameState game;
  game_init(&game);

  // Move player paddle up many times
  for (int i = 0; i < 30; i++) {
    game_handle_action(&game, ACT_UP);
  }
  assert(game.player.y >= 3.0f);

  // Move player paddle down many times
  for (int i = 0; i < 30; i++) {
    game_handle_action(&game, ACT_DOWN);
  }
  assert(game.player.y <= (float)(COURT_HEIGHT - 4));
  printf("[PASS] test_paddle_movement_and_clamping\n");
}

static void test_wall_bounce(void) {
  GameState game;
  game_init(&game);
  game.state = STATE_PLAYING;

  // 1. Top wall bounce
  game.ball.x = 20.0f;
  game.ball.y = 1.2f;
  game.ball.vx = 10.0f;
  game.ball.vy = -10.0f;

  game_update(&game, 0.05f);
  assert(game.ball.vy > 0.0f); // Velocity reversed downwards
  assert(game.ball.y >= 1.0f);

  // 2. Bottom wall bounce
  game.ball.y = (float)(COURT_HEIGHT - 2) - 0.2f;
  game.ball.vy = 10.0f;

  game_update(&game, 0.05f);
  assert(game.ball.vy < 0.0f); // Velocity reversed upwards
  assert(game.ball.y <= (float)(COURT_HEIGHT - 2));

  printf("[PASS] test_wall_bounce\n");
}

static void test_paddle_collision_and_deflection(void) {
  GameState game;
  game_init(&game);
  game.state = STATE_PLAYING;

  // Position player paddle at y = 10.0f
  game.player.y = 10.0f;

  // Aim ball towards player paddle (x = 2) at top corner of paddle (y = 8.5f)
  game.ball.x = 3.5f;
  game.ball.y = 8.5f;
  game.ball.vx = -20.0f;
  game.ball.vy = 0.0f;

  game_update(&game, 0.05f);

  // Ball should have bounced rightward and upward
  assert(game.ball.vx > 0.0f);
  assert(game.ball.vy < 0.0f);
  assert(game.rally_count == 1);
  printf("[PASS] test_paddle_collision_and_deflection\n");
}

static void test_cpu_ai_tracking(void) {
  GameState game;
  game_init(&game);
  game.state = STATE_PLAYING;

  // CPU paddle at center y = 11.0f
  game.cpu.y = 11.0f;

  // Ball moving towards CPU on upper half (y = 4.0f)
  game.ball.x = 30.0f;
  game.ball.y = 4.0f;
  game.ball.vx = 20.0f;
  game.ball.vy = 0.0f;

  float initial_cpu_y = game.cpu.y;
  game_update(&game, 0.1f);

  // CPU paddle should have tracked upwards
  assert(game.cpu.y < initial_cpu_y);
  printf("[PASS] test_cpu_ai_tracking\n");
}

static void test_scoring_and_match_win(void) {
  GameState game;
  game_init(&game);
  game.state = STATE_PLAYING;

  // Ball past left boundary -> CPU scores
  game.ball.x = 0.4f;
  game.ball.y = 10.0f;
  game.ball.vx = -10.0f;
  game_update(&game, 0.05f);

  assert(game.cpu_score == 1);
  assert(game.state == STATE_POINT_SCORED);

  // Force CPU score to WIN_SCORE - 1, then score again
  game.cpu_score = WIN_SCORE - 1;
  game.state = STATE_PLAYING;
  game.ball.x = 0.4f;
  game_update(&game, 0.05f);

  assert(game.cpu_score == WIN_SCORE);
  assert(game.state == STATE_GAME_OVER);
  printf("[PASS] test_scoring_and_match_win\n");
}

static void test_auto_save_and_resume(void) {
  game_delete_save();
  GameState g1;
  game_init(&g1);

  g1.player_score = 4;
  g1.cpu_score = 3;
  g1.rally_count = 12;
  g1.max_rally = 18;
  g1.player.y = 7.5f;
  g1.cpu.y = 14.2f;
  g1.ball.x = 34.0f;
  g1.ball.y = 8.5f;
  g1.ball.vx = 28.0f;
  g1.ball.vy = -6.0f;
  g1.state = STATE_PLAYING;

  game_save(&g1);

  // Re-open in a second instance
  GameState g2;
  game_init(&g2);

  assert(g2.player_score == 4);
  assert(g2.cpu_score == 3);
  assert(g2.rally_count == 12);
  assert(g2.max_rally == 18);
  assert(fabsf(g2.player.y - 7.5f) < 0.01f);
  assert(fabsf(g2.cpu.y - 14.2f) < 0.01f);
  assert(fabsf(g2.ball.x - 34.0f) < 0.01f);
  assert(fabsf(g2.ball.y - 8.5f) < 0.01f);

  game_delete_save();
  printf("[PASS] test_auto_save_and_resume\n");
}

int main(void) {
  srand(42);
  printf("Starting Classic Pong Logic Tests...\n");
  test_initialization();
  test_paddle_movement_and_clamping();
  test_wall_bounce();
  test_paddle_collision_and_deflection();
  test_cpu_ai_tracking();
  test_scoring_and_match_win();
  test_auto_save_and_resume();
  printf("All Pong tests PASSED successfully!\n");
  return 0;
}
