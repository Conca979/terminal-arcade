#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "game.h"

static void test_initialization(void) {
  GameState game;
  game_init(&game);

  assert(game.player.lives == 3);
  assert(game.swarm.alive_count == TOTAL_ALIENS);
  assert(game.score == 0);
  assert(game.wave == 1);
  assert(game.state == STATE_PLAYING);

  // Check bunkers initialized
  for (int i = 0; i < BUNKER_COUNT; i++) {
    for (int p = 0; p < BUNKER_PARTS; p++) {
      assert(game.bunkers[i].hp[p] == BUNKER_MAX_HP);
    }
  }
  printf("[PASS] test_initialization\n");
}

static void test_movement_and_bounds(void) {
  GameState game;
  game_init(&game);

  int initial_x = game.player.x;

  game_handle_action(&game, ACT_LEFT, 100);
  assert(game.player.x == initial_x - 2);

  game_handle_action(&game, ACT_RIGHT, 150);
  assert(game.player.x == initial_x);

  // Move far left
  for (int i = 0; i < 40; i++) {
    game_handle_action(&game, ACT_LEFT, 200 + i);
  }
  assert(game.player.x >= 2);
  printf("[PASS] test_movement_and_bounds\n");
}

static void test_bullet_firing(void) {
  GameState game;
  game_init(&game);

  // Fire bullet
  game_handle_action(&game, ACT_FIRE, 100);

  bool bullet_found = false;
  for (int i = 0; i < MAX_PLAYER_BULLETS; i++) {
    if (game.player_bullets[i].active) {
      bullet_found = true;
      assert(game.player_bullets[i].y == 21);
      assert(game.player_bullets[i].x == game.player.x);
      break;
    }
  }
  assert(bullet_found);
  printf("[PASS] test_bullet_firing\n");
}

static void test_alien_collision_and_scoring(void) {
  GameState game;
  game_init(&game);

  // Align bullet directly where alien will be after step
  int target_x = game.swarm.origin_x + game.swarm.dir_x + 1;
  game.player.x = target_x;

  int target_y = game.swarm.origin_y + 3 * 2;
  game.player_bullets[0].active = true;
  game.player_bullets[0].x = target_x;
  game.player_bullets[0].y = target_y;

  int initial_alive = game.swarm.alive_count;
  int initial_score = game.score;

  // Update game to process collisions
  game_update(&game, 500);

  // Bottom alien was hit
  assert(game.swarm.alive_count == initial_alive - 1);
  assert(game.score > initial_score);
  assert(!game.player_bullets[0].active);
  printf("[PASS] test_alien_collision_and_scoring (score = %d)\n", game.score);
}

static void test_bunker_erosion(void) {
  GameState game;
  game_init(&game);

  // Place player bullet hitting first bunker
  int bx = game.bunkers[0].x;
  int by = game.bunkers[0].y;
  game.player_bullets[0].active = true;
  game.player_bullets[0].x = bx;
  game.player_bullets[0].y = by;

  int initial_hp = game.bunkers[0].hp[0];
  game_update(&game, 500);

  assert(game.bunkers[0].hp[0] == initial_hp - 1);
  assert(!game.player_bullets[0].active);
  printf("[PASS] test_bunker_erosion\n");
}

int main(void) {
  srand(42);
  printf("Starting Space Invaders Logic Tests...\n");
  test_initialization();
  test_movement_and_bounds();
  test_bullet_firing();
  test_alien_collision_and_scoring();
  test_bunker_erosion();
  printf("All Space Invaders tests PASSED successfully!\n");
  return 0;
}
