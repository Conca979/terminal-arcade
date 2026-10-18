#ifndef CHESS_INPUT_H
#define CHESS_INPUT_H

#include <stdbool.h>
#include "config.h"

typedef struct {
  int x;
  int y;
  int button; /* 1 = left click, 2 = right click */
} MouseClick;

void input_init(void);
GameAction input_get_action(MouseClick *mouse);

#endif /* CHESS_INPUT_H */
