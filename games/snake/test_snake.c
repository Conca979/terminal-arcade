#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "game.h"

static void test_initialization(void) {
  GameState game;
  game_init(&game);

  assert(game.length == 4);
  assert(game.score == 0);
  assert(game.dir == DIR_RIGHT);
  assert(game.state == STATE_PLAYING);
  assert(game.regular_food.type == FOOD_DATA);
  printf("[PASS] test_initialization\n");
}

static void test_movement_and_growth(void) {
  GameState game;
  game_init(&game);

  int initial_head_x = game.body[0].x;

  // Place food right in front of snake
  game.regular_food.pos.x = initial_head_x + 1;
  game.regular_food.pos.y = game.body[0].y;

  // Advance time past tick interval
  game_update(&game, 200);

  // Snake should have eaten food and grown
  assert(game.body[0].x == initial_head_x + 1);
  assert(game.length == 5);
  assert(game.score == 10);
  printf("[PASS] test_movement_and_growth\n");
}

static void test_wall_collision_and_wrap(void) {
  GameState game;
  game_init(&game);

  // 1. Solid wall mode (default): moving into wall should trigger game over
  game.wrap_mode = false;
  game.body[0].x = ARENA_WIDTH - 1;
  game.body[0].y = 5;
  game.dir = DIR_RIGHT;
  game.next_dir = DIR_RIGHT;

  game_update(&game, 200);
  assert(game.state == STATE_GAME_OVER);

  // 2. Wrap mode: moving into wall should wrap to 0
  game_reset(&game);
  game.wrap_mode = true;
  game.body[0].x = ARENA_WIDTH - 1;
  game.body[0].y = 5;
  game.dir = DIR_RIGHT;
  game.next_dir = DIR_RIGHT;

  game_update(&game, 200);
  assert(game.state == STATE_PLAYING);
  assert(game.body[0].x == 0);
  assert(game.body[0].y == 5);

  printf("[PASS] test_wall_collision_and_wrap\n");
}

static void test_overclock_multiplier(void) {
  GameState game;
  game_init(&game);

  game.overclock_active = true;
  game.overclock_expire = 10000;

  // Place food in front of snake
  game.regular_food.pos.x = game.body[0].x + 1;
  game.regular_food.pos.y = game.body[0].y;

  game_update(&game, 200);

  // Normal 10 pts * 2x overclock = 20 pts
  assert(game.score == 20);
  printf("[PASS] test_overclock_multiplier\n");
}

int main(void) {
  srand(42);
  printf("Starting Cyberpunk Snake Logic Tests...\n");
  test_initialization();
  test_movement_and_growth();
  test_wall_collision_and_wrap();
  test_overclock_multiplier();
  printf("All Cyberpunk Snake tests PASSED successfully!\n");
  return 0;
}
