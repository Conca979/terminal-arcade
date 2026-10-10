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

  render_frame(&game);

  while (game.state != STATE_EXIT) {
    GameAction action = input_get_action();

    if (action != ACT_NONE) {
      switch (action) {
        case ACT_MOVE_UP:
          game_slide(&game, DIR_UP);
          break;

        case ACT_MOVE_DOWN:
          game_slide(&game, DIR_DOWN);
          break;

        case ACT_MOVE_LEFT:
          game_slide(&game, DIR_LEFT);
          break;

        case ACT_MOVE_RIGHT:
          game_slide(&game, DIR_RIGHT);
          break;

        case ACT_UNDO:
          game_undo(&game);
          break;

        case ACT_RESTART:
          game_reset(&game);
          break;

        case ACT_CONTINUE:
          if (game.state == STATE_WON) {
            game.won_acknowledged = true;
            game.state = STATE_PLAYING;
          }
          break;

        case ACT_QUIT:
          game.state = STATE_EXIT;
          break;

        default:
          break;
      }

      render_frame(&game);
    }

    platform_sleep_ms(16);
  }

  render_cleanup();
  platform_cleanup();

  return 0;
}
