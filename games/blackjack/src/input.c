#include "input.h"
#include <conio.h>

GameAction input_get_action(RoundState current_state) {
  if (!_kbhit()) {
    return ACT_NONE;
  }

  int ch = _getch();

  // Handle extended keys (arrows)
  if (ch == 0 || ch == 224) {
    int ext = _getch();
    if (current_state == STATE_BETTING) {
      if (ext == 72) { // Up
        return ACT_BET_INC;
      } else if (ext == 80) { // Down
        return ACT_BET_DEC;
      } else if (ext == 75) { // Left
        return ACT_BET_DEC;
      } else if (ext == 77) { // Right
        return ACT_BET_INC;
      }
    }
    return ACT_NONE;
  }

  // Universal exit keys: Esc (27), Ctrl+C (3), Ctrl+Q (17)
  if (ch == 27 || ch == 3 || ch == 17) {
    return ACT_QUIT;
  }

  // Quit with 'Q' or 'q'
  if (ch == 'q' || ch == 'Q') {
    return ACT_QUIT;
  }

  // Reload bankroll / reset with 'R' or 'r'
  if (ch == 'r' || ch == 'R') {
    return ACT_RELOAD;
  }

  if (current_state == STATE_BETTING) {
    switch (ch) {
      case 'w':
      case 'W':
        return ACT_BET_INC;
      case 's':
      case 'S':
        return ACT_BET_DEC;
      case '1':
        return ACT_BET_PRESET_1;
      case '2':
        return ACT_BET_PRESET_2;
      case '3':
        return ACT_BET_PRESET_3;
      case '4':
        return ACT_BET_PRESET_4;
      case '5':
        return ACT_BET_PRESET_5;
      case 'a':
      case 'A':
        return ACT_BET_ALL_IN;
      case ' ':
      case '\r':
      case '\n':
        return ACT_CONFIRM;
      default:
        return ACT_NONE;
    }
  } else if (current_state == STATE_INSURANCE) {
    switch (ch) {
      case 'y':
      case 'Y':
      case 'i':
      case 'I':
        return ACT_INSURE_YES;
      case 'n':
      case 'N':
      case ' ':
      case '\r':
      case '\n':
        return ACT_INSURE_NO;
      default:
        return ACT_NONE;
    }
  } else if (current_state == STATE_PLAYER_TURN) {
    switch (ch) {
      case 'h':
      case 'H':
      case ' ':
        return ACT_HIT;
      case 's':
      case 'S':
      case '\r':
      case '\n':
        return ACT_STAND;
      case 'd':
      case 'D':
        return ACT_DOUBLE;
      case 'p':
      case 'P':
      case '\t':
        return ACT_SPLIT;
      case 'u':
      case 'U':
        return ACT_SURRENDER;
      default:
        return ACT_NONE;
    }
  } else if (current_state == STATE_ROUND_OVER || current_state == STATE_GAME_OVER) {
    if (ch == ' ' || ch == '\r' || ch == '\n') {
      return ACT_CONFIRM;
    }
  }

  return ACT_NONE;
}
