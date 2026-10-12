#include "input.h"
#include <conio.h>
#include <ctype.h>

GameAction input_get_action(PlayState state, char *out_char) {
  if (out_char) {
    *out_char = '\0';
  }

  if (!_kbhit()) {
    return ACT_NONE;
  }

  int ch = _getch();

  // Handle special prefix keys
  if (ch == 0 || ch == 224) {
    int ext = _getch();
    (void)ext;
    return ACT_NONE;
  }

  // Universal exit keys: Esc (27), Ctrl+C (3), Ctrl+Q (17)
  if (ch == 27 || ch == 3 || ch == 17) {
    return ACT_QUIT;
  }

  // Ctrl+R (18): Force restart round
  if (ch == 18) {
    return ACT_RESTART;
  }

  // Backspace / Delete
  if (ch == '\b' || ch == 127) {
    return ACT_BACKSPACE;
  }

  // Enter
  if (ch == '\r' || ch == '\n') {
    if (state == STATE_PLAYING) {
      return ACT_SUBMIT;
    }
    return ACT_RESTART;
  }

  // Spacebar to restart when round is over
  if (ch == ' ') {
    if (state == STATE_WON || state == STATE_LOST) {
      return ACT_RESTART;
    }
    return ACT_NONE;
  }

  // In post-game screen, 'R' restarts and 'Q' quits
  if (state == STATE_WON || state == STATE_LOST) {
    if (ch == 'r' || ch == 'R') {
      return ACT_RESTART;
    }
    if (ch == 'q' || ch == 'Q') {
      return ACT_QUIT;
    }
    return ACT_NONE;
  }

  // During active gameplay, letters A-Z are typed
  if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')) {
    if (out_char) {
      *out_char = (char)toupper(ch);
    }
    return ACT_LETTER;
  }

  return ACT_NONE;
}
