#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "game.h"

static void test_initialization(void) {
  GameState game;
  game_delete_save();
  game_reset(&game);

  assert(game.round == 1);
  assert(game.score == 0);
  assert(game.total_balls == 1);
  assert(game.phase == PHASE_AIMING);
  assert(game.launcher_x >= 2.0f && game.launcher_x <= ARENA_WIDTH - 2.0f);
  assert(game.laser_guide.count > 0);

  // Check that row 0 has at least 1 brick
  int brick_count = 0;
  for (int c = 0; c < BRICK_COLS; c++) {
    if (game.grid[0][c].type != BRICK_NONE) {
      brick_count++;
    }
  }
  assert(brick_count > 0);
  printf("[PASS] test_initialization (round = %d, bricks = %d)\n", game.round, brick_count);
}

static void test_aiming_and_trajectory(void) {
  GameState game;
  game_reset(&game);

  // Aim upward-left (-2.0 radians)
  game_set_aim_angle(&game, -2.0f);
  assert(fabsf(game.aim_angle - (-2.0f)) < 0.001f);
  assert(game.laser_guide.count > 5);

  // First point should be at launcher
  assert(fabsf(game.laser_guide.points[0].x - game.launcher_x) < 0.01f);
  assert(fabsf(game.laser_guide.points[0].y - (ARENA_HEIGHT - 1.0f)) < 0.01f);

  printf("[PASS] test_aiming_and_trajectory (trajectory points = %d)\n", game.laser_guide.count);
}

static void test_firing_and_physics(void) {
  GameState game;
  game_reset(&game);

  game_fire(&game, 1000);
  assert(game.phase == PHASE_FIRING);
  assert(game.balls_to_launch == 1);
  assert(game.balls_active == 0);

  // Update physics to spawn ball
  game_update(&game, 1060);
  assert(game.balls_to_launch == 0);
  assert(game.balls_active == 1);

  // Run physics steps and assert ball moves upward
  float start_y = game.balls[0].y;
  for (int step = 0; step < 5; step++) {
    game_update(&game, 1080 + step * 16);
  }
  assert(game.balls[0].y < start_y);

  printf("[PASS] test_firing_and_physics (ball moving upward to y = %.2f)\n", game.balls[0].y);
}

static void test_brick_collision(void) {
  GameState game;
  game_reset(&game);

  // Place a test brick in col 4, row 5
  game.grid[5][4].type = BRICK_NORMAL;
  game.grid[5][4].hp = 1;
  game.grid[5][4].max_hp = 1;

  int initial_score = game.score;

  // Aim directly at col 4
  float target_x = 4.0f * BRICK_WIDTH + 3.0f;
  game.launcher_x = target_x;
  game_set_aim_angle(&game, -1.5707963f); // Straight up

  game_fire(&game, 1000);
  game_update(&game, 1060); // Launch ball

  // Step physics until ball hits row 5
  for (int f = 0; f < 100 && game.grid[5][4].type != BRICK_NONE; f++) {
    game_update(&game, 1080 + f * 16);
  }

  // Brick should be destroyed and score increased
  assert(game.grid[5][4].type == BRICK_NONE);
  assert(game.score > initial_score);

  printf("[PASS] test_brick_collision (brick shattered, score = %d)\n", game.score);
}

static void test_save_and_resuming(void) {
  GameState game;
  game_reset(&game);

  game.round = 15;
  game.score = 4250;
  game.total_balls = 15;
  game.launcher_x = 24.5f;
  game.grid[3][2].type = BRICK_NORMAL;
  game.grid[3][2].hp = 15;

  game_save(&game);

  // Load into fresh GameState
  GameState loaded;
  memset(&loaded, 0, sizeof(GameState));
  bool success = game_load(&loaded);

  assert(success);
  assert(loaded.round == 15);
  assert(loaded.score == 4250);
  assert(loaded.total_balls == 15);
  assert(fabsf(loaded.launcher_x - 24.5f) < 0.01f);
  assert(loaded.grid[3][2].type == BRICK_NORMAL);
  assert(loaded.grid[3][2].hp == 15);
  assert(loaded.resumed_game == true);

  game_delete_save();
  printf("[PASS] test_save_and_resuming (persisted Round %d, Balls %d)\n", loaded.round, loaded.total_balls);
}

int main(void) {
  printf("Starting Neon Balls Bricks Breaker Logic Tests...\n");
  test_initialization();
  test_aiming_and_trajectory();
  test_firing_and_physics();
  test_brick_collision();
  test_save_and_resuming();
  printf("All Breaker tests PASSED successfully!\n");
  return 0;
}
