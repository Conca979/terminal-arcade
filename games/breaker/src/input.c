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
        case VK_LEFT:
          return (InputEvent){ACT_AIM_LEFT, 0, 0};
        case VK_RIGHT:
          return (InputEvent){ACT_AIM_RIGHT, 0, 0};
        case VK_SPACE:
        case VK_RETURN:
          return (InputEvent){ACT_FIRE, 0, 0};
        case VK_ESCAPE:
          return (InputEvent){ACT_QUIT, 0, 0};
        default:
          break;
      }

      switch (ch) {
        case 'a':
        case 'A':
        case 'h':
        case 'H':
          return (InputEvent){ACT_AIM_LEFT, 0, 0};
        case 'd':
        case 'D':
        case 'l':
        case 'L':
          return (InputEvent){ACT_AIM_RIGHT, 0, 0};
        case 'f':
        case 'F':
          return (InputEvent){ACT_TURBO, 0, 0};
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
        return (InputEvent){ACT_FIRE, pos.X, pos.Y};
      } else if (btns & RIGHTMOST_BUTTON_PRESSED) {
        return (InputEvent){ACT_TURBO, pos.X, pos.Y};
      } else if (flags & MOUSE_MOVED) {
        return (InputEvent){ACT_MOUSE_MOVE, pos.X, pos.Y};
      }
    }
  }

  return (InputEvent){ACT_NONE, 0, 0};
}
