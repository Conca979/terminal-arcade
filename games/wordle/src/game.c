#include "game.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../../include/scores.h"
#include "../solution_words.c"

#define SAVE_MAGIC_WORDLE 0x5752444C // "WRDL"
#define SAVE_VERSION_WORDLE 1

static char VALID_WORDS[MAX_VALID_WORDS][WORD_LEN + 1];
static int VALID_WORDS_COUNT = 0;
static bool DICTIONARY_LOADED = false;

static const char *get_save_path(void) {
  FILE *f = fopen("games/wordle/solution_words.c", "r");
  if (f) {
    fclose(f);
    return "games/wordle/savegame.dat";
  }
  return "savegame.dat";
}

static const char *get_valid_words_path(void) {
  FILE *f = fopen("games/wordle/valid-wordle-words.txt", "r");
  if (f) {
    fclose(f);
    return "games/wordle/valid-wordle-words.txt";
  }
  f = fopen("valid-wordle-words.txt", "r");
  if (f) {
    fclose(f);
    return "valid-wordle-words.txt";
  }
  f = fopen("../games/wordle/valid-wordle-words.txt", "r");
  if (f) {
    fclose(f);
    return "../games/wordle/valid-wordle-words.txt";
  }
  f = fopen("../../games/wordle/valid-wordle-words.txt", "r");
  if (f) {
    fclose(f);
    return "../../games/wordle/valid-wordle-words.txt";
  }
  return "games/wordle/valid-wordle-words.txt";
}

static int compare_words(const void *a, const void *b) {
  return strcmp((const char *)a, (const char *)b);
}

int game_load_valid_dictionary(void) {
  if (DICTIONARY_LOADED) {
    return VALID_WORDS_COUNT;
  }

  const char *path = get_valid_words_path();
  FILE *f = fopen(path, "r");
  if (!f) {
    // Fallback: load solution words into valid words
    for (size_t i = 0; i < WORDLE_WORD_COUNT && i < MAX_VALID_WORDS; i++) {
      for (int c = 0; c < WORD_LEN; c++) {
        VALID_WORDS[i][c] = (char)tolower(WORDLE_WORDS[i][c]);
      }
      VALID_WORDS[i][WORD_LEN] = '\0';
      VALID_WORDS_COUNT++;
    }
    qsort(VALID_WORDS, VALID_WORDS_COUNT, sizeof(VALID_WORDS[0]), compare_words);
    DICTIONARY_LOADED = true;
    return VALID_WORDS_COUNT;
  }

  char line[64];
  while (fgets(line, sizeof(line), f)) {
    // Trim newline and whitespace
    char clean[WORD_LEN + 1];
    int len = 0;
    for (int i = 0; line[i] != '\0' && len < WORD_LEN; i++) {
      if (isalpha(line[i])) {
        clean[len++] = (char)tolower(line[i]);
      }
    }
    clean[len] = '\0';

    if (len == WORD_LEN && VALID_WORDS_COUNT < MAX_VALID_WORDS) {
      memcpy(VALID_WORDS[VALID_WORDS_COUNT], clean, WORD_LEN + 1);
      VALID_WORDS_COUNT++;
    }
  }
  fclose(f);

  qsort(VALID_WORDS, VALID_WORDS_COUNT, sizeof(VALID_WORDS[0]), compare_words);
  DICTIONARY_LOADED = true;
  return VALID_WORDS_COUNT;
}

bool game_is_valid_word(const char *word) {
  if (!word || strlen(word) != WORD_LEN) {
    return false;
  }

  if (!DICTIONARY_LOADED) {
    game_load_valid_dictionary();
  }

  char lower[WORD_LEN + 1];
  for (int i = 0; i < WORD_LEN; i++) {
    lower[i] = (char)tolower(word[i]);
  }
  lower[WORD_LEN] = '\0';

  if (VALID_WORDS_COUNT > 0) {
    void *found = bsearch(lower, VALID_WORDS, VALID_WORDS_COUNT, sizeof(VALID_WORDS[0]), compare_words);
    if (found) {
      return true;
    }
  }

  // Also check against solution words directly
  for (size_t i = 0; i < WORDLE_WORD_COUNT; i++) {
    if (strcmp(lower, WORDLE_WORDS[i]) == 0) {
      return true;
    }
  }

  return false;
}

void game_evaluate_guess(const char *guess, const char *target, TileStatus *out_status) {
  if (!guess || !target || !out_status) return;

  int target_counts[26] = {0};
  for (int i = 0; i < WORD_LEN; i++) {
    out_status[i] = TILE_ABSENT;
    char t = (char)toupper(target[i]);
    if (t >= 'A' && t <= 'Z') {
      target_counts[t - 'A']++;
    }
  }

  // Pass 1: exact matches (Green)
  for (int i = 0; i < WORD_LEN; i++) {
    char g = (char)toupper(guess[i]);
    char t = (char)toupper(target[i]);
    if (g == t) {
      out_status[i] = TILE_CORRECT;
      target_counts[g - 'A']--;
    }
  }

  // Pass 2: misplaced matches (Yellow)
  for (int i = 0; i < WORD_LEN; i++) {
    if (out_status[i] == TILE_CORRECT) continue;
    char g = (char)toupper(guess[i]);
    if (g >= 'A' && g <= 'Z' && target_counts[g - 'A'] > 0) {
      out_status[i] = TILE_PRESENT;
      target_counts[g - 'A']--;
    } else {
      out_status[i] = TILE_ABSENT;
    }
  }
}

static void update_key_statuses(GameState *game, int row) {
  for (int i = 0; i < WORD_LEN; i++) {
    char c = (char)toupper(game->guesses[row].letters[i]);
    if (c >= 'A' && c <= 'Z') {
      int idx = c - 'A';
      TileStatus ts = game->guesses[row].status[i];
      KeyStatus current = game->key_status[idx];

      if (ts == TILE_CORRECT) {
        game->key_status[idx] = KEY_CORRECT;
      } else if (ts == TILE_PRESENT && current != KEY_CORRECT) {
        game->key_status[idx] = KEY_PRESENT;
      } else if (ts == TILE_ABSENT && current == KEY_UNUSED) {
        game->key_status[idx] = KEY_ABSENT;
      }
    }
  }
}

void game_load_stats(GameState *game) {
  game->games_played = arcade_score_get("wordle", "total_played");
  game->games_won = arcade_score_get("wordle", "games_won");
  game->current_streak = arcade_score_get("wordle", "win_streak");
  game->max_streak = arcade_score_get("wordle", "max_streak");

  for (int i = 0; i < MAX_ATTEMPTS; i++) {
    char key[16];
    snprintf(key, sizeof(key), "dist_%d", i + 1);
    game->guess_distribution[i] = arcade_score_get("wordle", key);
  }
}

void game_save_stats(const GameState *game) {
  arcade_score_set("wordle", "total_played", game->games_played);
  arcade_score_set("wordle", "games_won", game->games_won);
  arcade_score_set("wordle", "win_streak", game->current_streak);
  arcade_score_set("wordle", "max_streak", game->max_streak);

  for (int i = 0; i < MAX_ATTEMPTS; i++) {
    char key[16];
    snprintf(key, sizeof(key), "dist_%d", i + 1);
    arcade_score_set("wordle", key, game->guess_distribution[i]);
  }
}

void game_delete_save(void) {
  remove(get_save_path());
}

void game_save(const GameState *game) {
  if (game->state != STATE_PLAYING) {
    return;
  }

  FILE *f = fopen(get_save_path(), "wb");
  if (!f) return;

  uint32_t magic = SAVE_MAGIC_WORDLE;
  uint32_t ver = SAVE_VERSION_WORDLE;
  fwrite(&magic, sizeof(uint32_t), 1, f);
  fwrite(&ver, sizeof(uint32_t), 1, f);

  fwrite(game->target_word, sizeof(char), WORD_LEN + 1, f);
  fwrite(game->guesses, sizeof(GuessRow), MAX_ATTEMPTS, f);
  fwrite(&game->current_attempt, sizeof(int), 1, f);
  fwrite(&game->current_letter_idx, sizeof(int), 1, f);
  fwrite(game->key_status, sizeof(KeyStatus), 26, f);

  fclose(f);
}

bool game_load(GameState *game) {
  FILE *f = fopen(get_save_path(), "rb");
  if (!f) return false;

  uint32_t magic = 0, ver = 0;
  if (fread(&magic, sizeof(uint32_t), 1, f) != 1 || magic != SAVE_MAGIC_WORDLE) {
    fclose(f);
    return false;
  }
  if (fread(&ver, sizeof(uint32_t), 1, f) != 1 || ver != SAVE_VERSION_WORDLE) {
    fclose(f);
    return false;
  }

  if (fread(game->target_word, sizeof(char), WORD_LEN + 1, f) != (WORD_LEN + 1) ||
      fread(game->guesses, sizeof(GuessRow), MAX_ATTEMPTS, f) != MAX_ATTEMPTS ||
      fread(&game->current_attempt, sizeof(int), 1, f) != 1 ||
      fread(&game->current_letter_idx, sizeof(int), 1, f) != 1 ||
      fread(game->key_status, sizeof(KeyStatus), 26, f) != 26) {
    fclose(f);
    return false;
  }

  fclose(f);

  game->state = STATE_PLAYING;
  game->message[0] = '\0';
  game->message_timer_ms = 0;

  return true;
}

void game_set_target_word(GameState *game, const char *word) {
  if (!game || !word) return;
  for (int i = 0; i < WORD_LEN; i++) {
    game->target_word[i] = (char)toupper(word[i]);
  }
  game->target_word[WORD_LEN] = '\0';
}

void game_new_round(GameState *game) {
  game_delete_save();

  // Pick random solution word
  if (WORDLE_WORD_COUNT > 0) {
    int idx = rand() % (int)WORDLE_WORD_COUNT;
    game_set_target_word(game, WORDLE_WORDS[idx]);
  } else {
    game_set_target_word(game, "CRANE");
  }

  for (int r = 0; r < MAX_ATTEMPTS; r++) {
    memset(game->guesses[r].letters, 0, sizeof(game->guesses[r].letters));
    for (int c = 0; c < WORD_LEN; c++) {
      game->guesses[r].status[c] = TILE_EMPTY;
    }
  }

  for (int i = 0; i < 26; i++) {
    game->key_status[i] = KEY_UNUSED;
  }

  game->current_attempt = 0;
  game->current_letter_idx = 0;
  game->state = STATE_PLAYING;
  game->message[0] = '\0';
  game->message_timer_ms = 0;
}

void game_init(GameState *game) {
  memset(game, 0, sizeof(GameState));
  game_load_valid_dictionary();
  game_load_stats(game);

  if (!game_load(game)) {
    game_new_round(game);
  }
}

bool game_submit_guess(GameState *game) {
  if (game->state != STATE_PLAYING) {
    return false;
  }

  if (game->current_letter_idx < WORD_LEN) {
    snprintf(game->message, sizeof(game->message), "NOT ENOUGH LETTERS");
    game->message_timer_ms = 1800;
    return false;
  }

  GuessRow *row = &game->guesses[game->current_attempt];
  row->letters[WORD_LEN] = '\0';

  if (!game_is_valid_word(row->letters)) {
    snprintf(game->message, sizeof(game->message), "NOT IN WORD LIST");
    game->message_timer_ms = 1800;
    return false;
  }

  // Valid guess, evaluate tiles
  game_evaluate_guess(row->letters, game->target_word, row->status);
  update_key_statuses(game, game->current_attempt);

  // Check victory
  if (strcmp(row->letters, game->target_word) == 0) {
    game->state = STATE_WON;
    game->games_played++;
    game->games_won++;
    game->current_streak++;
    if (game->current_streak > game->max_streak) {
      game->max_streak = game->current_streak;
    }
    if (game->current_attempt >= 0 && game->current_attempt < MAX_ATTEMPTS) {
      game->guess_distribution[game->current_attempt]++;
    }

    const char *compliments[] = {
      "GENIUS!", "MAGNIFICENT!", "IMPRESSIVE!", "SPLENDID!", "GREAT!", "PHEW!"
    };
    snprintf(game->message, sizeof(game->message), "%s", compliments[game->current_attempt]);
    game->message_timer_ms = 60000; // Persist banner until new round

    game_save_stats(game);
    game_delete_save();
    return true;
  }

  // Check loss
  if (game->current_attempt >= MAX_ATTEMPTS - 1) {
    game->state = STATE_LOST;
    game->games_played++;
    game->current_streak = 0;

    snprintf(game->message, sizeof(game->message), "WORD WAS: %s", game->target_word);
    game->message_timer_ms = 60000;

    game_save_stats(game);
    game_delete_save();
    return true;
  }

  // Advance to next row
  game->current_attempt++;
  game->current_letter_idx = 0;
  game->message[0] = '\0';
  game->message_timer_ms = 0;

  game_save(game);
  return true;
}

void game_handle_action(GameState *game, GameAction action, char letter) {
  switch (action) {
    case ACT_LETTER:
      if (game->state == STATE_PLAYING && game->current_letter_idx < WORD_LEN) {
        game->guesses[game->current_attempt].letters[game->current_letter_idx] = (char)toupper(letter);
        game->guesses[game->current_attempt].status[game->current_letter_idx] = TILE_TYPING;
        game->current_letter_idx++;
        game->guesses[game->current_attempt].letters[game->current_letter_idx] = '\0';
      }
      break;

    case ACT_BACKSPACE:
      if (game->state == STATE_PLAYING && game->current_letter_idx > 0) {
        game->current_letter_idx--;
        game->guesses[game->current_attempt].letters[game->current_letter_idx] = '\0';
        game->guesses[game->current_attempt].status[game->current_letter_idx] = TILE_EMPTY;
      }
      break;

    case ACT_SUBMIT:
      if (game->state == STATE_PLAYING) {
        game_submit_guess(game);
      }
      break;

    case ACT_RESTART:
      game_new_round(game);
      break;

    case ACT_QUIT:
      if (game->state == STATE_PLAYING) {
        game_save(game);
      }
      game->state = STATE_EXIT;
      break;

    default:
      break;
  }
}

void game_update(GameState *game, float dt) {
  if (game->message_timer_ms > 0 && game->state == STATE_PLAYING) {
    game->message_timer_ms -= (int)(dt * 1000.0f);
    if (game->message_timer_ms <= 0) {
      game->message[0] = '\0';
      game->message_timer_ms = 0;
    }
  }
}
