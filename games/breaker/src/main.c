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

  const int target_frame_ms = 16; // 60 FPS fluid physics loop

  while (game.phase != PHASE_EXIT) {
    unsigned long long frame_start = platform_get_time_ms();

    InputEvent ev;
    while ((ev = input_poll_event()).action != ACT_NONE) {
      game_handle_action(&game, ev.action, ev.mouse_x, ev.mouse_y, frame_start);
      if (game.phase == PHASE_EXIT) {
        break;
      }
    }

    if (game.phase != PHASE_EXIT) {
      game_update(&game, frame_start);
      render_frame(&game, frame_start);
    }

    unsigned long long frame_end = platform_get_time_ms();
    int elapsed = (int)(frame_end - frame_start);
    if (elapsed < target_frame_ms) {
      platform_sleep_ms(target_frame_ms - elapsed);
    }
  }

  // Auto-save on exit if still in an active run
  if (game.phase != PHASE_GAME_OVER) {
    game_save(&game);
  }

  render_cleanup();
  platform_cleanup();

  return 0;
}
