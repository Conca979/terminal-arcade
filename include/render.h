#ifndef RENDER_H
#define RENDER_H

#include "game.h"

void render_init(void);
void render_frame(const GameState *game);
void render_cleanup(void);

#endif /* RENDER_H */
