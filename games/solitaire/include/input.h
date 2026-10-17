#ifndef INPUT_SOLITAIRE_H
#define INPUT_SOLITAIRE_H

#include <stdbool.h>
#include "config.h"

typedef struct {
  int x;
  int y;
  int button; /* 1 = left click, 2 = right click */
} MouseClick;

void input_init(void);
GameAction input_get_action(MouseClick *mouse);

#endif /* INPUT_SOLITAIRE_H */

