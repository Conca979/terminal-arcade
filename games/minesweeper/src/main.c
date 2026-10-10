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

  unsigned long long now = platform_get_time_ms();
  render_frame(&game, now);

  unsigned long long last_render = now;

  while (game.state != STATE_EXIT) {
    now = platform_get_time_ms();
    GameAction action = input_get_action();
    bool state_changed = false;

    if (action != ACT_NONE) {
      state_changed = true;
      switch (action) {
        case ACT_UP:
          game_move_cursor(&game, -1, 0);
          break;

        case ACT_DOWN:
          game_move_cursor(&game, 1, 0);
          break;

        case ACT_LEFT:
          game_move_cursor(&game, 0, -1);
          break;

        case ACT_RIGHT:
          game_move_cursor(&game, 0, 1);
          break;

        case ACT_REVEAL:
          game_reveal(&game, game.cursor_r, game.cursor_c, now);
          break;

        case ACT_FLAG:
          game_toggle_flag(&game, game.cursor_r, game.cursor_c);
          break;

        case ACT_CHORD:
          game_chord(&game, game.cursor_r, game.cursor_c, now);
          break;

        case ACT_DIFF_1:
          game_set_difficulty(&game, DIFF_BEGINNER);
          break;

        case ACT_DIFF_2:
          game_set_difficulty(&game, DIFF_INTERMEDIATE);
          break;

        case ACT_DIFF_3:
          game_set_difficulty(&game, DIFF_EXPERT);
          break;

        case ACT_RESTART:
          game_reset(&game);
          break;

        case ACT_QUIT:
          game.state = STATE_EXIT;
          break;

        default:
          break;
      }
    }

    // Refresh every second if timer is ticking or when state changes
    if (state_changed || (game.state == STATE_PLAYING && (now - last_render >= 1000))) {
      render_frame(&game, now);
      last_render = now;
    }

    platform_sleep_ms(16);
  }

  render_cleanup();
  platform_cleanup();

  return 0;
}
