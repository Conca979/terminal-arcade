#include "input.h"
#include <windows.h>

InputEvent input_poll_event(void) {
  HANDLE h_stdin = GetStdHandle(STD_INPUT_HANDLE);
  if (h_stdin == INVALID_HANDLE_VALUE) {
    return (InputEvent){ACT_NONE, 0, 0};
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
          return (InputEvent){ACT_UP, 0, 0};
        case VK_DOWN:
          return (InputEvent){ACT_DOWN, 0, 0};
        case VK_LEFT:
          return (InputEvent){ACT_LEFT, 0, 0};
        case VK_RIGHT:
          return (InputEvent){ACT_RIGHT, 0, 0};
        case VK_SPACE:
        case VK_RETURN:
          return (InputEvent){ACT_PLACE, 0, 0};
        case VK_ESCAPE:
          return (InputEvent){ACT_DESELECT, 0, 0};
        default:
          break;
      }

      switch (ch) {
        case 'w':
        case 'W':
        case 'k':
        case 'K':
          return (InputEvent){ACT_UP, 0, 0};
        case 's':
        case 'S':
        case 'j':
        case 'J':
          return (InputEvent){ACT_DOWN, 0, 0};
        case 'a':
        case 'A':
        case 'h':
        case 'H':
          return (InputEvent){ACT_LEFT, 0, 0};
        case 'd':
        case 'D':
        case 'l':
        case 'L':
          return (InputEvent){ACT_RIGHT, 0, 0};
        case '1':
          return (InputEvent){ACT_SELECT_1, 0, 0};
        case '2':
          return (InputEvent){ACT_SELECT_2, 0, 0};
        case '3':
          return (InputEvent){ACT_SELECT_3, 0, 0};
        case 'x':
        case 'X':
        case 'c':
        case 'C':
          return (InputEvent){ACT_DESELECT, 0, 0};
        case 'r':
        case 'R':
          return (InputEvent){ACT_RESTART, 0, 0};
        case 'q':
        case 'Q':
          return (InputEvent){ACT_QUIT, 0, 0};
        default:
          break;
      }
    } else if (rec.EventType == MOUSE_EVENT) {
      COORD pos = rec.Event.MouseEvent.dwMousePosition;
      DWORD btns = rec.Event.MouseEvent.dwButtonState;
      DWORD flags = rec.Event.MouseEvent.dwEventFlags;

      if (btns & FROM_LEFT_1ST_BUTTON_PRESSED) {
        return (InputEvent){ACT_MOUSE_CLICK, pos.X, pos.Y};
      } else if (btns & RIGHTMOST_BUTTON_PRESSED) {
        return (InputEvent){ACT_DESELECT, pos.X, pos.Y};
      } else if (flags & MOUSE_MOVED) {
        return (InputEvent){ACT_MOUSE_MOVE, pos.X, pos.Y};
      }
    }
  }

  return (InputEvent){ACT_NONE, 0, 0};
}
