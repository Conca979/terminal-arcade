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
      case 72: return ACT_UP;
      case 80: return ACT_DOWN;
      case 75: return ACT_LEFT;
      case 77: return ACT_RIGHT;
      case 83: return ACT_CLEAR; // Delete key
      default: return ACT_NONE;
    }
  }

  if (ch >= '1' && ch <= '9') {
    return (GameAction)(ACT_NUM_1 + (ch - '1'));
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

    case 'a':
    case 'A':
      return ACT_LEFT;

    case 'd':
    case 'D':
      return ACT_RIGHT;

    case '0':
    case 8:   // Backspace
    case 127: // Delete
    case 'x':
    case 'X':
      return ACT_CLEAR;

    case 'n':
    case 'N':
      return ACT_TOGGLE_NOTE_MODE;

    case 'u':
    case 'U':
    case 'z':
    case 'Z':
      return ACT_UNDO;

    case 'h':
    case 'H':
      return ACT_HINT;

    case 'e':
    case 'E':
      return ACT_TOGGLE_ERRORS;

    case 'm':
    case 'M':
    case '\t': // Tab cycles difficulty
      return ACT_NEXT_DIFF;

    case 'r':
    case 'R':
      return ACT_RESTART;

    case 'p':
    case 'P':
      return ACT_PAUSE;

    case 'q':
    case 'Q':
    case 27: // ESC
      return ACT_QUIT;

    default:
      return ACT_NONE;
  }
}
