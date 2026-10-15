#include <stdbool.h>
#include <stdio.h>
#include "game.h"
#include "input.h"
#include "platform.h"
#include "render.h"

int main(void) {
  platform_init();
  render_init();

  GameState game;
  game_init(&game);

  while (!game.should_quit) {
    render_game(&game);

    GameAction action = input_get_action(game.state);
    if (action != ACT_NONE) {
      game_handle_action(&game, action);
    }

    platform_sleep_ms(16);
  }

  game_save_scores(&game);
  render_cleanup();
  platform_cleanup();

  return 0;
}
