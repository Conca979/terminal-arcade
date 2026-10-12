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
  unsigned long long last_frame_time = platform_get_time_ms();

  while (game.state != STATE_EXIT) {
    unsigned long long frame_start = platform_get_time_ms();
    float dt = (float)(frame_start - last_frame_time) / 1000.0f;
    if (dt <= 0.0f || dt > 0.05f) {
      dt = 0.016f;
    }
    last_frame_time = frame_start;

    GameAction action;
    while ((action = input_get_action()).type != ACT_NONE) {
      game_handle_action(&game, action);
      if (game.state == STATE_EXIT) {
        break;
      }
    }

    if (game.state != STATE_EXIT) {
      game_update(&game, dt);
      render_frame(&game);
    }

    unsigned long long frame_end = platform_get_time_ms();
    int elapsed = (int)(frame_end - frame_start);
    if (elapsed < target_frame_ms) {
      platform_sleep_ms(target_frame_ms - elapsed);
    }
  }

  if (game.state != STATE_GAME_OVER) {
    game_save(&game);
  }

  render_cleanup();
  platform_cleanup();

  return 0;
}
