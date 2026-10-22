#include "config.h"
#include "game.h"
#include "input.h"
#include "platform.h"
#include "render.h"

int main(void) {
  platform_init();
  render_init();

  GameState game;
  game_init(&game, THEME_SUNSET);

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

      case ACT_TAB:
        game_toggle_focus(&game);
        break;

      case ACT_SELECT:
        game_interact_current_focus(&game);
        break;

      case ACT_RETURN:
        if (game.holding_piece != -1) {
          game_return_holding_to_tray(&game);
        } else if (game.focus == FOCUS_BOARD) {
          game_return_slot_to_tray(&game, game.cursor_board_r, game.cursor_board_c);
        }
        break;

      case ACT_HINT:
        game_request_hint(&game);
        break;

      case ACT_THEME:
        game_next_theme(&game);
        break;

      case ACT_PREVIEW:
        game_toggle_preview(&game);
        break;

      case ACT_UNDO:
        game_undo(&game);
        break;

      case ACT_RESTART:
        game_reset(&game, game.current_theme);
        break;

      case ACT_MOUSE_CLICK: {
        int r = -1, c = -1, t_idx = -1;
        if (render_get_board_slot_at_pos(mouse.x, mouse.y, &r, &c)) {
          game.cursor_board_r = r;
          game.cursor_board_c = c;
          game.focus = FOCUS_BOARD;
          game_place_on_board(&game, r, c);
        } else if (render_get_tray_slot_at_pos(mouse.x, mouse.y, &t_idx)) {
          game.cursor_tray_idx = t_idx;
          game.focus = FOCUS_TRAY;
          int piece_id = game.tray_order[t_idx];
          if (game.tray_present[piece_id]) {
            game_pick_from_tray(&game, piece_id);
          } else if (game.holding_piece != -1) {
            game_return_holding_to_tray(&game);
          }
        }
        break;
      }

      case ACT_MOUSE_RIGHT: {
        if (game.holding_piece != -1) {
          game_return_holding_to_tray(&game);
        } else {
          int r = -1, c = -1;
          if (render_get_board_slot_at_pos(mouse.x, mouse.y, &r, &c)) {
            game_return_slot_to_tray(&game, r, c);
          }
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
