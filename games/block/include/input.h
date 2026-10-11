#ifndef BLOCK_INPUT_H
#define BLOCK_INPUT_H

#include "game.h"

typedef struct {
  GameAction action;
  int mouse_x;
  int mouse_y;
} InputEvent;

InputEvent input_poll_event(void);

#endif // BLOCK_INPUT_H
