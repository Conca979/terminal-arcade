#ifndef LIFE_INPUT_H
#define LIFE_INPUT_H

#include "config.h"

typedef struct {
  int x;
  int y;
  int button; /* 1 = left, 2 = right */
} MouseClick;

void input_init(void);
GameAction input_get_action(MouseClick *mouse);

#endif /* LIFE_INPUT_H */
