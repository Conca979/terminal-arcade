#ifndef RENDER_MINESWEEPER_H
#define RENDER_MINESWEEPER_H

#include "game.h"

void render_init(void);
void render_frame(const GameState *game, unsigned long long current_time);
void render_cleanup(void);

#endif /* RENDER_MINESWEEPER_H */
