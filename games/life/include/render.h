#ifndef LIFE_RENDER_H
#define LIFE_RENDER_H

#include "game.h"
#include <stdbool.h>

void render_init(void);
void render_cleanup(void);
void render_game(const GameState *g);

bool render_get_cell_at_pos(int mouse_x, int mouse_y, int *out_r, int *out_c);

#endif /* LIFE_RENDER_H */
