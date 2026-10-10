#ifndef RENDER_SNAKE_H
#define RENDER_SNAKE_H

#include "game.h"

void render_init(void);
void render_frame(const GameState *game, unsigned long long current_time);
void render_cleanup(void);

#endif /* RENDER_SNAKE_H */
