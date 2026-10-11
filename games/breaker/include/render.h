#ifndef BREAKER_RENDER_H
#define BREAKER_RENDER_H

#include "game.h"

void render_init(void);
void render_cleanup(void);
void render_frame(const GameState *game, unsigned long long current_time);

#endif // BREAKER_RENDER_H
