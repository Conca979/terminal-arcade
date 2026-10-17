#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include "config.h"
#include "game.h"
#include "input.h"
#include "platform.h"
#include "render.h"

int main(void) {
  platform_init();
  render_init();

  GameState game;
  game_init(&game, (unsigned int)time(NULL));

  unsigned long long last_sec_tick = platform_get_time_ms();
  bool running = true;

  while (running) {
    unsigned long long frame_start = platform_get_time_ms();

    // 1. Process Input
    MouseClick mouse = { 0, 0, 0 };
    GameAction act = input_get_action(&mouse);

    switch (act) {
      case ACT_DRAW:
        game_draw_stock(&game);
        break;

      case ACT_SELECT_WASTE:
        game_select_pile(&game, PILE_WASTE);
        break;

      case ACT_SELECT_TABLEAU_1:
      case ACT_SELECT_TABLEAU_2:
      case ACT_SELECT_TABLEAU_3:
      case ACT_SELECT_TABLEAU_4:
      case ACT_SELECT_TABLEAU_5:
      case ACT_SELECT_TABLEAU_6:
      case ACT_SELECT_TABLEAU_7: {
        PileType pt = (PileType)(PILE_TABLEAU_1 + (act - ACT_SELECT_TABLEAU_1));
        game_select_pile(&game, pt);
        break;
      }

      case ACT_SELECT_FOUNDATION_AUTO:
        if (game.selected_pile != PILE_NONE) {
          game_auto_foundation_for_pile(&game, game.selected_pile);
          game_clear_selection(&game);
        } else {
          game_auto_move(&game);
        }
        break;

      case ACT_NAV_UP:
      case ACT_NAV_DOWN:
      case ACT_NAV_LEFT:
      case ACT_NAV_RIGHT:
        game_nav_cursor(&game, act);
        break;

      case ACT_CONFIRM:
        if (game.cursor_pile == PILE_STOCK) {
          game_draw_stock(&game);
        } else if (game.cursor_pile == PILE_WASTE || (game.cursor_pile >= PILE_FOUNDATION_1 && game.cursor_pile <= PILE_FOUNDATION_4)) {
          game_select_pile(&game, game.cursor_pile);
        } else if (game.cursor_pile >= PILE_TABLEAU_1 && game.cursor_pile <= PILE_TABLEAU_7) {
          int col = game.cursor_pile - PILE_TABLEAU_1;
          game_select_tableau_card(&game, col, game.cursor_card_idx);
        }
        break;

      case ACT_CANCEL:
        game_clear_selection(&game);
        snprintf(game.status_msg, sizeof(game.status_msg), "Selection cleared");
        break;

      case ACT_AUTO_MOVE:
        game_auto_move(&game);
        break;

      case ACT_UNDO:
        game_undo(&game);
        break;

      case ACT_HINT:
        game_get_hint(&game, game.status_msg, sizeof(game.status_msg));
        break;

      case ACT_NEW_GAME:
        game_init(&game, (unsigned int)time(NULL));
        break;

      case ACT_RESTART:
        game_restart(&game);
        break;

      case ACT_QUIT:
        running = false;
        break;

      case ACT_MOUSE_CLICK: {
        int card_idx = -1;
        PileType clicked = render_get_pile_at_pos(mouse.x, mouse.y, &card_idx);
        if (clicked != PILE_NONE) {
          if (mouse.button == 1) { // Left click
            game.cursor_pile = clicked;
            if (clicked == PILE_STOCK) {
              game_draw_stock(&game);
            } else if (clicked >= PILE_TABLEAU_1 && clicked <= PILE_TABLEAU_7) {
              game_select_pile(&game, clicked);
            } else {
              game_select_pile(&game, clicked);
            }
          } else if (mouse.button == 2) { // Right click: instant foundation
            game_auto_foundation_for_pile(&game, clicked);
          }
        }
        break;
      }

      default:
        break;
    }

    // 2. Timer Update
    unsigned long long now = platform_get_time_ms();
    if (now - last_sec_tick >= 1000) {
      if (game.status == STATE_PLAYING) {
        game.time_elapsed_sec++;
      }
      last_sec_tick = now;
    }

    // 3. Render Frame
    render_game(&game);

    // 4. Frame rate pacing (~60 FPS)
    unsigned long long frame_dur = platform_get_time_ms() - frame_start;
    if (frame_dur < 16) {
      platform_sleep_ms((int)(16 - frame_dur));
    }
  }

  game_save_scores(&game);
  render_cleanup();
  platform_cleanup();
  return 0;
}

