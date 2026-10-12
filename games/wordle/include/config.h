#ifndef CONFIG_WORDLE_H
#define CONFIG_WORDLE_H

#define WORD_LEN 5
#define MAX_ATTEMPTS 6
#define MAX_VALID_WORDS 16000

typedef enum {
  TILE_EMPTY = 0,
  TILE_TYPING,
  TILE_ABSENT,  // Letter not in word (Gray)
  TILE_PRESENT, // Letter in word, wrong position (Yellow)
  TILE_CORRECT  // Letter in word, correct position (Green)
} TileStatus;

typedef enum {
  KEY_UNUSED = 0,
  KEY_ABSENT,  // Dark Gray
  KEY_PRESENT, // Yellow
  KEY_CORRECT  // Green
} KeyStatus;

typedef enum {
  STATE_PLAYING,
  STATE_WON,
  STATE_LOST,
  STATE_EXIT
} PlayState;

typedef enum {
  ACT_NONE = 0,
  ACT_LETTER,
  ACT_BACKSPACE,
  ACT_SUBMIT,
  ACT_RESTART,
  ACT_QUIT
} GameAction;

#endif /* CONFIG_WORDLE_H */
