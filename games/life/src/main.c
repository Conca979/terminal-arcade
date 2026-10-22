#include "config.h"
#include "game.h"
#include "input.h"
#include "platform.h"
#include "render.h"

int main(void) {
  platform_init();
  render_init();

  GameState game;
  game_init(&game);

  bool running = true;
  double last_time = platform_get_time();

  while (running) {
    double current_time = platform_get_time();
    double dt = current_time - last_time;
    last_time = current_time;

    MouseClick mouse = { 0, 0, 0 };
    GameAction act = input_get_action(&mouse);

    switch (act) {
      case ACT_QUIT:
        running = false;
        break;

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

      case ACT_PLAY_PAUSE:
        game_toggle_pause(&game);
        break;

      case ACT_STEP_FORWARD:
        game_step_forward(&game);
        break;

      case ACT_STEP_BACK:
        game_step_backward(&game);
        break;

      case ACT_TOGGLE_CELL:
        game_toggle_cell(&game, game.cursor_r, game.cursor_c);
        break;

      case ACT_CLEAR:
        game_clear(&game);
        break;

      case ACT_RANDOMIZE:
        game_randomize(&game, 25);
        break;

      case ACT_PRESET_CYCLE:
        game_cycle_preset(&game);
        break;

      case ACT_STAMP_PRESET:
        game_stamp_preset(&game, game.cursor_r, game.cursor_c, game.selected_preset);
        break;

      case ACT_WRAP_TOGGLE:
        game_toggle_wrap(&game);
        break;

      case ACT_SPEED_UP:
        game_adjust_speed(&game, 1);
        break;

      case ACT_SPEED_DOWN:
        game_adjust_speed(&game, -1);
        break;

      case ACT_MOUSE_CLICK: {
        int r = -1, c = -1;
        if (render_get_cell_at_pos(mouse.x, mouse.y, &r, &c)) {
          game.cursor_r = r;
          game.cursor_c = c;
          game_toggle_cell(&game, r, c);
        }
        break;
      }

      case ACT_MOUSE_RIGHT: {
        int r = -1, c = -1;
        if (render_get_cell_at_pos(mouse.x, mouse.y, &r, &c)) {
          game.cursor_r = r;
          game.cursor_c = c;
          game_set_cell(&game, r, c, false);
        }
        break;
      }

      default:
        break;
    }

    game_update(&game, dt);
    render_game(&game);

    platform_sleep_ms(16);
  }

  game_save_scores(&game);
  render_cleanup();
  platform_cleanup();

  return 0;
}
