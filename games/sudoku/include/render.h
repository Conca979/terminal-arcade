#ifndef SUDOKU_RENDER_H
#define SUDOKU_RENDER_H

#include "game.h"

void render_init(void);
void render_cleanup(void);
void render_frame(const GameState *game, unsigned long long current_time);

#endif // SUDOKU_RENDER_H
