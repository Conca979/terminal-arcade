#ifndef INPUT_PONG_H
#define INPUT_PONG_H

#include "config.h"

GameAction input_get_action(void);
void input_poll_continuous_movement(float *player_vy);

#endif /* INPUT_PONG_H */
