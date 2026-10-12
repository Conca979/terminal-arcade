#include "input.h"
#include <conio.h>
#include <windows.h>

GameAction input_get_action(void) {
  if (!_kbhit()) {
    return ACT_NONE;
  }

  int ch = _getch();

  if (ch == 0 || ch == 224) {
    int ext = _getch();
    switch (ext) {
      case 72: return ACT_UP;
      case 80: return ACT_DOWN;
      default: return ACT_NONE;
    }
  }

  switch (ch) {
    case 'w':
    case 'W':
    case 'k':
    case 'K':
      return ACT_UP;

    case 's':
    case 'S':
    case 'j':
    case 'J':
      return ACT_DOWN;

    case ' ':
    case '\r':
    case '\n':
      return ACT_SERVE;

    case '\t':
    case 'm':
    case 'M':
      return ACT_TOGGLE_DIFF;

    case 'p':
    case 'P':
      return ACT_PAUSE;

    case 'r':
    case 'R':
      return ACT_RESTART;

    case 'q':
    case 'Q':
    case 27: // ESC
      return ACT_QUIT;

    default:
      return ACT_NONE;
  }
}

void input_poll_continuous_movement(float *player_vy) {
  bool up_pressed = (GetAsyncKeyState('W') & 0x8000) ||
                    (GetAsyncKeyState(VK_UP) & 0x8000) ||
                    (GetAsyncKeyState('K') & 0x8000);

  bool down_pressed = (GetAsyncKeyState('S') & 0x8000) ||
                      (GetAsyncKeyState(VK_DOWN) & 0x8000) ||
                      (GetAsyncKeyState('J') & 0x8000);

  if (up_pressed && !down_pressed) {
    *player_vy = -PLAYER_PADDLE_SPEED;
  } else if (down_pressed && !up_pressed) {
    *player_vy = PLAYER_PADDLE_SPEED;
  } else {
    *player_vy = 0.0f;
  }
}
