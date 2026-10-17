#include "input.h"
#include <windows.h>

void input_init(void) {
  // Input initialization is handled in platform_init
}

GameAction input_get_action(MouseClick *mouse) {
  HANDLE h_stdin = GetStdHandle(STD_INPUT_HANDLE);
  if (h_stdin == INVALID_HANDLE_VALUE) {
    return ACT_NONE;
  }

  INPUT_RECORD rec;
  DWORD num_events = 0;

  while (PeekConsoleInput(h_stdin, &rec, 1, &num_events) && num_events > 0) {
    if (!ReadConsoleInput(h_stdin, &rec, 1, &num_events)) {
      break;
    }

    if (rec.EventType == KEY_EVENT) {
      if (!rec.Event.KeyEvent.bKeyDown) {
        continue;
      }

      WORD vk = rec.Event.KeyEvent.wVirtualKeyCode;
      char ch = rec.Event.KeyEvent.uChar.AsciiChar;

      switch (vk) {
        case VK_UP:
          return ACT_NAV_UP;
        case VK_DOWN:
          return ACT_NAV_DOWN;
        case VK_LEFT:
          return ACT_NAV_LEFT;
        case VK_RIGHT:
          return ACT_NAV_RIGHT;
        case VK_RETURN:
          return ACT_CONFIRM;
        case VK_ESCAPE:
          return ACT_QUIT;
        case VK_BACK:
          return ACT_CANCEL;
        default:
          break;
      }

      switch (ch) {
        case 's':
        case 'S':
          return ACT_DRAW;
        case 'w':
        case 'W':
          return ACT_SELECT_WASTE;
        case '1': return ACT_SELECT_TABLEAU_1;
        case '2': return ACT_SELECT_TABLEAU_2;
        case '3': return ACT_SELECT_TABLEAU_3;
        case '4': return ACT_SELECT_TABLEAU_4;
        case '5': return ACT_SELECT_TABLEAU_5;
        case '6': return ACT_SELECT_TABLEAU_6;
        case '7': return ACT_SELECT_TABLEAU_7;
        case 'f':
        case 'F':
          return ACT_SELECT_FOUNDATION_AUTO;
        case 'a':
        case 'A':
          return ACT_AUTO_MOVE;
        case 'u':
        case 'U':
          return ACT_UNDO;
        case 'h':
        case 'H':
          return ACT_HINT;
        case 'n':
        case 'N':
          return ACT_NEW_GAME;
        case 'r':
        case 'R':
          return ACT_RESTART;
        case 'q':
        case 'Q':
          return ACT_QUIT;
        case 'x':
        case 'X':
        case 'c':
        case 'C':
          return ACT_CANCEL;
        case ' ':
          return ACT_CONFIRM;
        default:
          break;
      }
    } else if (rec.EventType == MOUSE_EVENT) {
      COORD pos = rec.Event.MouseEvent.dwMousePosition;
      DWORD btns = rec.Event.MouseEvent.dwButtonState;
      DWORD flags = rec.Event.MouseEvent.dwEventFlags;

      // Only respond to actual button clicks, not moves
      if (flags == 0) {
        if (btns & FROM_LEFT_1ST_BUTTON_PRESSED) {
          if (mouse) {
            mouse->x = pos.X;
            mouse->y = pos.Y;
            mouse->button = 1;
          }
          return ACT_MOUSE_CLICK;
        } else if (btns & RIGHTMOST_BUTTON_PRESSED) {
          if (mouse) {
            mouse->x = pos.X;
            mouse->y = pos.Y;
            mouse->button = 2;
          }
          return ACT_MOUSE_CLICK;
        }
      }
    }
  }

  return ACT_NONE;
}

