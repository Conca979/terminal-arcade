#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "config.h"
#include "game.h"
#include "input.h"
#include "platform.h"
#include "render.h"

int main(void) {
  srand((unsigned int)time(NULL));
  platform_init();
  input_init();
  render_init();

  GameState game;
  game_init(&game, AI_MEDIUM);

  bool running = true;

  while (running) {
    unsigned long long frame_start = platform_get_time_ms();

    /* If it is the AI's turn in an active game, render the board and let AI play */
    if (game.status == STATUS_PLAYING &&
        game.difficulty != AI_TWO_PLAYER &&
        game.turn != game.human_color &&
        !game.awaiting_promotion) {
      render_game(&game);
      platform_sleep_ms(120);
      game_ai_step(&game);
    }

    MouseClick mouse = { 0, 0, 0 };
    GameAction act = input_get_action(&mouse);

    if (game.awaiting_promotion) {
      switch (act) {
        case ACT_PROMO_Q:
        case ACT_SELECT:
          game_complete_promotion(&game, PIECE_QUEEN);
          break;
        case ACT_PROMO_R:
          game_complete_promotion(&game, PIECE_ROOK);
          break;
        case ACT_PROMO_B:
          game_complete_promotion(&game, PIECE_BISHOP);
          break;
        case ACT_PROMO_N:
          game_complete_promotion(&game, PIECE_KNIGHT);
          break;
        case ACT_CANCEL:
          game_cancel_selection(&game);
          break;
        case ACT_QUIT:
          running = false;
          break;
        default:
          break;
      }
    } else {
      switch (act) {
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
        case ACT_SELECT:
          game_select_square(&game, game.cursor_r, game.cursor_c);
          break;
        case ACT_CANCEL:
          game_cancel_selection(&game);
          break;
        case ACT_UNDO:
          game_undo(&game);
          break;
        case ACT_HINT:
          game_get_hint(&game, game.status_msg, sizeof(game.status_msg));
          break;
        case ACT_DIFFICULTY:
          game_cycle_difficulty(&game);
          break;
        case ACT_FLIP_BOARD:
          game.board_flipped = !game.board_flipped;
          snprintf(game.status_msg, sizeof(game.status_msg), "Board view flipped");
          break;
        case ACT_NEW_GAME:
          game_reset_board(&game);
          break;
        case ACT_QUIT:
          running = false;
          break;
        case ACT_MOUSE_CLICK: {
          if (mouse.button == 2) {
            game_cancel_selection(&game);
          } else if (mouse.button == 1) {
            int r = -1;
            int c = -1;
            if (render_get_square_at_pos(&game, mouse.x, mouse.y, &r, &c)) {
              game_select_square(&game, r, c);
            }
          }
          break;
        }
        default:
          break;
      }
    }

    render_game(&game);

    unsigned long long elapsed = platform_get_time_ms() - frame_start;
    if (elapsed < 16) {
      platform_sleep_ms((int)(16 - elapsed));
    }
  }

  game_save_scores(&game);
  render_cleanup();
  platform_cleanup();
  return 0;
}
