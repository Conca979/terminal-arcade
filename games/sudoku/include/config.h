#ifndef SUDOKU_CONFIG_H
#define SUDOKU_CONFIG_H

#define GRID_SIZE 9
#define BOX_SIZE 3
#define TOTAL_CELLS 81
#define MAX_UNDO_HISTORY 64

typedef enum {
  DIFF_EASY = 0,
  DIFF_MEDIUM = 1,
  DIFF_HARD = 2,
  DIFF_EXPERT = 3,
  DIFF_COUNT = 4
} DifficultyLevel;

#endif // SUDOKU_CONFIG_H
