#ifndef BREAKER_CONFIG_H
#define BREAKER_CONFIG_H

#define ARENA_WIDTH 48      // 8 brick columns * 6 chars per brick
#define ARENA_HEIGHT 34     // High-res vertical flight arena
#define BRICK_COLS 8        // Number of brick columns across
#define BRICK_ROWS 24       // Total rows in brick descent buffer
#define BRICK_WIDTH 6       // Character width of each brick: [ 128 ]
#define DANGER_ROW 24       // Row threshold: bricks reaching here trigger Game Over

#define MAX_BALLS 512       // Maximum ball swarm capacity
#define BALL_SPEED 0.85f    // Base floating point flight speed
#define TURBO_SPEED 2.40f   // 3x Fast-forward speed
#define LAUNCH_DELAY_MS 50  // Delay between consecutive ball launches

#define SAVE_FILE "savegame.dat"

#endif // BREAKER_CONFIG_H
