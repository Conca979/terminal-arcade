#ifndef CHESS_RENDER_H
#define CHESS_RENDER_H

#include <stdbool.h>
#include "game.h"

void render_init(void);
void render_cleanup(void);
void render_game(const GameState *g);
bool render_get_square_at_pos(const GameState *g, int mouse_x, int mouse_y, int *out_r, int *out_c);

#endif /* CHESS_RENDER_H */
