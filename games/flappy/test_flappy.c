#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "game.h"
#include "render.h"

static void test_initialization(void) {
  game_delete_save();
  GameState game;
  game_init(&game);

  assert(game.state == STATE_READY);
  assert(game.score == 0);
  assert(fabsf(game.bird.y - BIRD_START_Y) < 0.1f);
  assert(game.bird.vy == 0.0f);

  // Check pipes are spaced and have valid gaps
  for (int i = 0; i < MAX_PIPES; i++) {
    assert(game.pipes[i].gap_y >= 2);
    assert(game.pipes[i].gap_y <= SKY_HEIGHT - PIPE_GAP - 2);
    assert(!game.pipes[i].passed);
  }

  printf("[PASS] test_initialization\n");
}

static void test_flap_impulse_and_gravity(void) {
  GameState game;
  game_init(&game);
  game_reset(&game);

  // Flap from ready state starts the game
  game_handle_action(&game, ACT_FLAP);
  assert(game.state == STATE_PLAYING);
  assert(fabsf(game.bird.vy - FLAP_IMPULSE) < 0.01f);

  // Update with delta time
  float initial_vy = game.bird.vy;
  game_update(&game, 0.05f);

  // Velocity should have increased downwards due to gravity
  assert(game.bird.vy > initial_vy);

  // Terminal velocity clamping test
  for (int i = 0; i < 50; i++) {
    game_update(&game, 0.05f);
  }
  assert(game.bird.vy <= MAX_FALL_SPEED);

  printf("[PASS] test_flap_impulse_and_gravity\n");
}

static void test_ceiling_clamping(void) {
  GameState game;
  game_init(&game);
  game_reset(&game);
  game.state = STATE_PLAYING;

  // Position bird near ceiling and apply upward impulse
  game.bird.y = 0.2f;
  game.bird.vy = FLAP_IMPULSE;
  game_update(&game, 0.05f);

  // Bird position must be clamped at ceiling (0.0f) with velocity stopped
  assert(game.bird.y == 0.0f);
  assert(game.bird.vy >= 0.0f);

  // Repeated upward thrusts keep bird clamped at ceiling
  for (int i = 0; i < 10; i++) {
    game.bird.vy = FLAP_IMPULSE;
    game_update(&game, 0.05f);
    assert(game.bird.y == 0.0f);
  }

  printf("[PASS] test_ceiling_clamping\n");
}

static void test_ground_collision(void) {
  GameState game;
  game_init(&game);
  game_reset(&game);
  game.state = STATE_PLAYING;

  // Let bird fall to ground
  game.bird.y = (float)(SKY_HEIGHT - 2);
  game.bird.vy = 10.0f;

  game_update(&game, 0.2f);
  assert(game.state == STATE_GAME_OVER);

  printf("[PASS] test_ground_collision\n");
}

static void test_pipe_collision_detection(void) {
  GameState game;
  game_init(&game);
  game_reset(&game);
  game.state = STATE_PLAYING;

  // Place pipe 0 directly over bird's X position
  game.pipes[0].x = BIRD_X;
  game.pipes[0].gap_y = 5; // Gap from y=5 to y=9 inclusive

  // 1. Bird inside gap (y = 6.0f) -> NO collision
  game.bird.y = 6.0f;
  assert(!game_check_collision(&game));

  // 2. Bird hits top pipe (y = 3.0f < gap_y 5) -> COLLISION
  game.bird.y = 3.0f;
  assert(game_check_collision(&game));

  // 3. Bird hits bottom pipe (y = 11.0f >= gap_y + PIPE_GAP) -> COLLISION
  game.bird.y = 11.0f;
  assert(game_check_collision(&game));

  printf("[PASS] test_pipe_collision_detection\n");
}

static void test_pipe_scoring_progression(void) {
  GameState game;
  game_init(&game);
  game_reset(&game);
  game.state = STATE_PLAYING;

  // Position pipe 0 just past the bird
  game.pipes[0].x = BIRD_X - (float)PIPE_WIDTH - 0.5f;
  game.pipes[0].passed = false;
  game.score = 0;

  game_update(&game, 0.016f);

  assert(game.pipes[0].passed == true);
  assert(game.score == 1);

  // Test medal tier assignment
  assert(game_get_medal(5) == MEDAL_NONE);
  assert(game_get_medal(12) == MEDAL_BRONZE_TIER);
  assert(game_get_medal(25) == MEDAL_SILVER_TIER);
  assert(game_get_medal(35) == MEDAL_GOLD_TIER);
  assert(game_get_medal(45) == MEDAL_PLATINUM_TIER);

  printf("[PASS] test_pipe_scoring_progression\n");
}

static void test_pause_and_restart(void) {
  GameState game;
  game_init(&game);
  game_reset(&game);
  game.state = STATE_PLAYING;
  game.score = 4;
  game.bird.y = 6.0f;
  game.bird.vy = 2.0f;

  // Toggle pause
  game_handle_action(&game, ACT_PAUSE);
  assert(game.state == STATE_PAUSED);

  // Update during pause should not change bird physics
  game_update(&game, 0.1f);
  assert(fabsf(game.bird.y - 6.0f) < 0.001f);
  assert(fabsf(game.bird.vy - 2.0f) < 0.001f);

  // Unpause
  game_handle_action(&game, ACT_PAUSE);
  assert(game.state == STATE_PLAYING);

  // Restart
  game_handle_action(&game, ACT_RESTART);
  assert(game.state == STATE_READY);
  assert(game.score == 0);

  printf("[PASS] test_pause_and_restart\n");
}

static void test_auto_save_and_resume(void) {
  game_delete_save();
  GameState g1;
  game_init(&g1);
  game_reset(&g1);

  g1.state = STATE_PLAYING;
  g1.score = 7;
  g1.bird.y = 8.5f;
  g1.bird.vy = -3.2f;
  g1.pipes[0].x = 24.0f;
  g1.pipes[0].gap_y = 4;

  game_save(&g1);

  // Load into second instance
  GameState g2;
  game_init(&g2);

  assert(g2.score == 7);
  assert(fabsf(g2.bird.y - 8.5f) < 0.01f);
  assert(fabsf(g2.bird.vy - (-3.2f)) < 0.01f);
  assert(fabsf(g2.pipes[0].x - 24.0f) < 0.01f);
  assert(g2.pipes[0].gap_y == 4);
  // Resumes in paused state to protect player from instant death
  assert(g2.state == STATE_PAUSED);

  game_delete_save();
  printf("[PASS] test_auto_save_and_resume\n");
}

static void test_render_frame_memory_safety(void) {
  GameState game;
  game_init(&game);
  game_reset(&game);
  game.score = 35;
  game.high_score = 42;
  game.medal = MEDAL_GOLD_TIER;
  game.state = STATE_PLAYING;

  volatile int canary = 0x5A5A5A5A;

  // Render multiple frames across different states
  render_frame(&game);

  game.state = STATE_GAME_OVER;
  render_frame(&game);

  game.state = STATE_READY;
  render_frame(&game);

  game.state = STATE_PAUSED;
  render_frame(&game);

  assert(canary == 0x5A5A5A5A);
  assert(game.score == 35);
  printf("[PASS] test_render_frame_memory_safety\n");
}

int main(void) {
  srand(42);
  printf("Starting Flappy Bird Logic & Physics Tests...\n");
  test_initialization();
  test_flap_impulse_and_gravity();
  test_ceiling_clamping();
  test_ground_collision();
  test_pipe_collision_detection();
  test_pipe_scoring_progression();
  test_pause_and_restart();
  test_auto_save_and_resume();
  test_render_frame_memory_safety();
  printf("All Flappy Bird tests PASSED successfully!\n");
  return 0;
}
