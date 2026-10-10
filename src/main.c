#include <stdlib.h>
#include <time.h>
#include "game.h"
#include "input.h"
#include "platform.h"
#include "render.h"

int main(void) {
  srand((unsigned int)time(NULL));

  platform_init();
  render_init();

  GameState game;
  game_init(&game);

  const int target_frame_ms = 16; // ~60 FPS

  while (game.state != STATE_EXIT) {
    unsigned long long frame_start = platform_get_time_ms();

    // Consume all pending inputs for low latency responsiveness
    GameAction action;
    while ((action = input_get_action()) != ACT_NONE) {
      game_handle_action(&game, action, frame_start);
      if (game.state == STATE_EXIT) {
        break;
      }
    }

    if (game.state != STATE_EXIT) {
      game_update(&game, frame_start);
      render_frame(&game);
    }

    unsigned long long frame_end = platform_get_time_ms();
    int elapsed = (int)(frame_end - frame_start);
    if (elapsed < target_frame_ms) {
      platform_sleep_ms(target_frame_ms - elapsed);
    }
  }

  render_cleanup();
  platform_cleanup();

  return 0;
}
