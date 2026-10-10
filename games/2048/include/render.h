#ifndef RENDER_2048_H
#define RENDER_2048_H

#include "game.h"

void render_init(void);
void render_frame(const GameState *game);
void render_cleanup(void);

#endif /* RENDER_2048_H */
