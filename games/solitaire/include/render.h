#ifndef RENDER_SOLITAIRE_H
#define RENDER_SOLITAIRE_H

#include "game.h"

void render_init(void);
void render_cleanup(void);
void render_game(const GameState *game);
PileType render_get_pile_at_pos(const GameState *game, int mouse_x, int mouse_y, int *out_card_idx);

#endif /* RENDER_SOLITAIRE_H */

