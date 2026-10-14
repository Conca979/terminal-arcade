#include "input.h"
#include <conio.h>

GameAction input_get_action(void) {
  if (!_kbhit()) {
    return ACT_NONE;
  }

  int ch = _getch();

  // Handle extended keys (arrow keys)
  if (ch == 0 || ch == 224) {
    int ext = _getch();
    switch (ext) {
      case 72: // Up Arrow
        return ACT_FLAP;
      default:
        return ACT_NONE;
    }
  }

  // Universal exit keys: Esc (27), Ctrl+C (3), Ctrl+Q (17)
  if (ch == 27 || ch == 3 || ch == 17) {
    return ACT_QUIT;
  }

  switch (ch) {
    case ' ':
    case 'w':
    case 'W':
    case '\r':
    case '\n':
      return ACT_FLAP;

    case 'p':
    case 'P':
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
