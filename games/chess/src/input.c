#include "input.h"
#include <windows.h>

void input_init(void) {
  /* Console input flags are initialized in platform_init */
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
        case VK_UP:     return ACT_UP;
        case VK_DOWN:   return ACT_DOWN;
        case VK_LEFT:   return ACT_LEFT;
        case VK_RIGHT:  return ACT_RIGHT;
        case VK_RETURN: return ACT_SELECT;
        case VK_ESCAPE: return ACT_QUIT;
        case VK_BACK:   return ACT_CANCEL;
        default:        break;
      }

      switch (ch) {
        case 'w':
        case 'W': return ACT_UP;
        case 's':
        case 'S': return ACT_DOWN;
        case 'a':
        case 'A': return ACT_LEFT;
        case 'd':
        case 'D': return ACT_RIGHT;
        case ' ': return ACT_SELECT;
        case 'x':
        case 'X':
        case 'c':
        case 'C': return ACT_CANCEL;
        case 'u':
        case 'U':
        case 'z':
        case 'Z': return ACT_UNDO;
        case 'h':
        case 'H': return ACT_HINT;
        case 'm':
        case 'M': return ACT_DIFFICULTY;
        case 'f':
        case 'F': return ACT_FLIP_BOARD;
        case 'n':
        case 'N':
        case 'r':
        case 'R': return ACT_NEW_GAME;
        case 'q':
        case 'Q': return ACT_QUIT;
        case '1': return ACT_PROMO_Q;
        case '2': return ACT_PROMO_R;
        case '3': return ACT_PROMO_B;
        case '4': return ACT_PROMO_N;
        default:  break;
      }
    } else if (rec.EventType == MOUSE_EVENT) {
      COORD pos = rec.Event.MouseEvent.dwMousePosition;
      DWORD btns = rec.Event.MouseEvent.dwButtonState;
      DWORD flags = rec.Event.MouseEvent.dwEventFlags;

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
