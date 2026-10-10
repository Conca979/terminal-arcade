#include "input.h"
#include <conio.h>

GameAction input_get_action(void) {
  if (!_kbhit()) {
    return ACT_NONE;
  }

  int ch = _getch();

  if (ch == 0 || ch == 224) {
    int ext = _getch();
    switch (ext) {
      case 75: return ACT_LEFT;
      case 77: return ACT_RIGHT;
      case 72: return ACT_FIRE; // Up arrow fires
      default: return ACT_NONE;
    }
  }

  switch (ch) {
    case 'a':
    case 'A':
      return ACT_LEFT;

    case 'd':
    case 'D':
      return ACT_RIGHT;

    case ' ':
    case 'w':
    case 'W':
      return ACT_FIRE;

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
