#ifndef RENDER_TYPING_H
#define RENDER_TYPING_H

#include "game.h"

void render_init(void);
void render_cleanup(void);
void render_frame(const GameState *game);

#endif /* RENDER_TYPING_H */
