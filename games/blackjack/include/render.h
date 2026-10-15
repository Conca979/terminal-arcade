#ifndef RENDER_BLACKJACK_H
#define RENDER_BLACKJACK_H

#include "game.h"

void render_init(void);
void render_cleanup(void);
void render_game(const GameState *game);

#endif /* RENDER_BLACKJACK_H */
