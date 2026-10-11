#ifndef BREAKER_INPUT_H
#define BREAKER_INPUT_H

#include "game.h"

typedef struct {
  GameAction action;
  int mouse_x;
  int mouse_y;
} InputEvent;

InputEvent input_poll_event(void);

#endif // BREAKER_INPUT_H
