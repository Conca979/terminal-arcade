#include "input.h"
#include <conio.h>

GameAction input_get_action(void) {
  if (!_kbhit()) {
    return ACT_NONE;
  }

  int ch = _getch();

  // Check for extended keys (arrow keys)
  if (ch == 0 || ch == 224) {
    int ext = _getch();
    switch (ext) {
      case 72: // Up arrow
        return ACT_MOVE_UP;
      case 80: // Down arrow
        return ACT_MOVE_DOWN;
      case 75: // Left arrow
        return ACT_MOVE_LEFT;
      case 77: // Right arrow
        return ACT_MOVE_RIGHT;
      default:
        return ACT_NONE;
    }
  }

  switch (ch) {
    case 'w':
    case 'W':
    case 'k':
    case 'K':
      return ACT_MOVE_UP;

    case 's':
    case 'S':
    case 'j':
    case 'J':
      return ACT_MOVE_DOWN;

    case 'a':
    case 'A':
    case 'h':
    case 'H':
      return ACT_MOVE_LEFT;

    case 'd':
    case 'D':
    case 'l':
    case 'L':
      return ACT_MOVE_RIGHT;

    case 'u':
    case 'U':
    case 'z':
    case 'Z':
      return ACT_UNDO;

    case 'r':
    case 'R':
      return ACT_RESTART;

    case 'c':
    case 'C':
    case '\r':
    case '\n':
    case ' ':
      return ACT_CONTINUE;

    case 'q':
    case 'Q':
    case 27: // ESC
      return ACT_QUIT;

    default:
      return ACT_NONE;
  }
}
