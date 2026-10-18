#ifndef CHESS_CONFIG_H
#define CHESS_CONFIG_H

#define BOARD_SIZE 8
#define MAX_MOVES 256
#define MAX_HISTORY 256

#define INNER_WIDTH 94
#define VIEWPORT_LINES 25

typedef enum {
  COLOR_NONE = 0,
  COLOR_WHITE = 1,
  COLOR_BLACK = 2
} PieceColor;

typedef enum {
  PIECE_NONE = 0,
  PIECE_PAWN = 1,
  PIECE_KNIGHT = 2,
  PIECE_BISHOP = 3,
  PIECE_ROOK = 4,
  PIECE_QUEEN = 5,
  PIECE_KING = 6
} PieceType;

typedef enum {
  AI_EASY = 0,
  AI_MEDIUM = 1,
  AI_HARD = 2,
  AI_TWO_PLAYER = 3
} AIDifficulty;

typedef enum {
  STATUS_PLAYING = 0,
  STATUS_CHECKMATE = 1,
  STATUS_STALEMATE = 2,
  STATUS_DRAW_50_MOVE = 3,
  STATUS_DRAW_MATERIAL = 4
} GameStatus;

typedef enum {
  ACT_NONE = 0,
  ACT_UP,
  ACT_DOWN,
  ACT_LEFT,
  ACT_RIGHT,
  ACT_SELECT,
  ACT_CANCEL,
  ACT_UNDO,
  ACT_HINT,
  ACT_DIFFICULTY,
  ACT_FLIP_BOARD,
  ACT_NEW_GAME,
  ACT_QUIT,
  ACT_PROMO_Q,
  ACT_PROMO_R,
  ACT_PROMO_B,
  ACT_PROMO_N,
  ACT_MOUSE_CLICK
} GameAction;

extern const int PIECE_VALUES[7];
extern const int PST_PAWN[8][8];
extern const int PST_KNIGHT[8][8];
extern const int PST_BISHOP[8][8];
extern const int PST_ROOK[8][8];
extern const int PST_QUEEN[8][8];
extern const int PST_KING_MID[8][8];
extern const int PST_KING_END[8][8];

#endif /* CHESS_CONFIG_H */
