#include "input.h"
#include <conio.h>

GameAction input_get_action(void) {
  if (!_kbhit()) {
    return ACT_NONE;
  }

  int ch = _getch();

  // Check for extended keys (arrow keys, etc.)
  if (ch == 0 || ch == 224) {
    int ext = _getch();
    switch (ext) {
      case 72: // Up arrow
        return ACT_ROTATE_CW;
      case 80: // Down arrow
        return ACT_DOWN;
      case 75: // Left arrow
        return ACT_LEFT;
      case 77: // Right arrow
        return ACT_RIGHT;
      default:
        return ACT_NONE;
    }
  }

  switch (ch) {
    case 'a':
    case 'A':
      return ACT_LEFT;

    case 'd':
    case 'D':
      return ACT_RIGHT;

    case 's':
    case 'S':
      return ACT_DOWN;

    case 'w':
    case 'W':
    case 'x':
    case 'X':
      return ACT_ROTATE_CW;

    case 'z':
    case 'Z':
      return ACT_ROTATE_CCW;

    case ' ':
      return ACT_HARD_DROP;

    case 'c':
    case 'C':
    case 'h':
    case 'H':
    case '0':
      return ACT_HOLD;

    case 'p':
    case 'P':
    case 27: // ESC
      return ACT_PAUSE;

    case 'r':
    case 'R':
      return ACT_RESTART;

    case 'q':
    case 'Q':
      return ACT_QUIT;

    default:
      return ACT_NONE;
  }
}
