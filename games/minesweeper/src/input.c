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

    case 'a':
    case 'A':
    case 'h':
    case 'H':
      return ACT_LEFT;

    case 'd':
    case 'D':
    case 'l':
    case 'L':
      return ACT_RIGHT;

    case ' ':
    case '\r':
    case '\n':
      return ACT_REVEAL;

    case 'f':
    case 'F':
      return ACT_FLAG;

    case 'c':
    case 'C':
      return ACT_CHORD;

    case '1':
      return ACT_DIFF_1;

    case '2':
      return ACT_DIFF_2;

    case '3':
      return ACT_DIFF_3;

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
