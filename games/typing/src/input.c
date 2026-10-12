#include "input.h"
#include <conio.h>
#include <ctype.h>

GameAction input_get_action(void) {
  GameAction act;
  act.type = ACT_NONE;
  act.ch = '\0';

  if (!_kbhit()) {
    return act;
  }

  int ch = _getch();

  if (ch == 0 || ch == 224) {
    _getch(); // Discard extended key scan code
    return act;
  }

  if (ch == 27 || ch == 17) { // ESC or Ctrl+Q
    act.type = ACT_QUIT;
    return act;
  }

  if (ch == 8 || ch == 127) { // Backspace or Del
    act.type = ACT_BACKSPACE;
    return act;
  }

  if (ch == ' ' || ch == '\r' || ch == '\n') {
    act.type = ACT_RESTART;
    return act;
  }

  if (isalpha(ch)) {
    act.type = ACT_CHAR;
    act.ch = (char)tolower(ch);
    return act;
  }

  return act;
}
