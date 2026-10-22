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
        case VK_TAB:    return ACT_TAB;
        case VK_ESCAPE: return ACT_QUIT;
        case VK_BACK:
        case VK_DELETE: return ACT_RETURN;
        default:        break;
      }

      switch (ch) {
        case 'w':
        case 'W':
        case 'k':
        case 'K': return ACT_UP;
        case 's':
        case 'S':
        case 'j':
        case 'J': return ACT_DOWN;
        case 'a':
        case 'A':
        case 'h':
        case 'H': return ACT_LEFT;
        case 'd':
        case 'D':
        case 'l':
        case 'L': return ACT_RIGHT;
        case ' ': return ACT_SELECT;
        case '\t': return ACT_TAB;
        case 'x':
        case 'X':
        case 'c':
        case 'C': return ACT_RETURN;
        case '?': return ACT_HINT;
        case 't':
        case 'T':
        case 'm':
        case 'M': return ACT_THEME;
        case 'p':
        case 'P': return ACT_PREVIEW;
        case 'u':
        case 'U':
        case 'z':
        case 'Z': return ACT_UNDO;
        case 'r':
        case 'R': return ACT_RESTART;
        case 'q':
        case 'Q': return ACT_QUIT;
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
          return ACT_MOUSE_RIGHT;
        }
      }
    }
  }

  return ACT_NONE;
}
