#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "game.h"

static void test_initialization_and_dictionary(void) {
  game_delete_save();
  GameState game;
  game_init(&game);

  assert(game.current_attempt == 0);
  assert(game.current_letter_idx == 0);
  assert(game.state == STATE_PLAYING);
  assert(strlen(game.target_word) == WORD_LEN);

  // Validate dictionary loading
  assert(game_is_valid_word("crane"));
  assert(game_is_valid_word("apple"));
  assert(game_is_valid_word("about"));
  assert(!game_is_valid_word("zzzzz"));
  assert(!game_is_valid_word("abc"));
  assert(!game_is_valid_word("longword"));

  printf("[PASS] test_initialization_and_dictionary\n");
}

static void test_two_pass_clue_evaluation(void) {
  TileStatus status[WORD_LEN];

  // Test 1: Exact match
  game_evaluate_guess("CRANE", "CRANE", status);
  for (int i = 0; i < WORD_LEN; i++) {
    assert(status[i] == TILE_CORRECT);
  }

  // Test 2: Target ROBOT, Guess BOBBY
  // ROBOT has 1 B (pos 2), 2 O (pos 1, 3), 1 R, 1 T.
  // In BOBBY: pos 2 is exact B (Green). The single B in ROBOT is consumed.
  // Thus pos 0 (B) is Gray (Absent), pos 3 (B) is Gray (Absent).
  game_evaluate_guess("BOBBY", "ROBOT", status);
  assert(status[0] == TILE_ABSENT);
  assert(status[1] == TILE_CORRECT);
  assert(status[2] == TILE_CORRECT);
  assert(status[3] == TILE_ABSENT);
  assert(status[4] == TILE_ABSENT);

  // Test 2b: Target ABBEY (two B's at pos 1, 2), Guess BOBBY
  // Pos 2 is exact B (Green). 1 B remains in ABBEY.
  // Pos 0 consumes the remaining B (Yellow / Present).
  // Pos 3 is Absent. Pos 4 (Y) is exact (Green).
  game_evaluate_guess("BOBBY", "ABBEY", status);
  assert(status[0] == TILE_PRESENT);
  assert(status[1] == TILE_ABSENT);
  assert(status[2] == TILE_CORRECT);
  assert(status[3] == TILE_ABSENT);
  assert(status[4] == TILE_CORRECT);

  // Test 3: Target ALERT, Guess AGREE
  // ALERT has A(0), L(1), E(2), R(3), T(4)
  // AGREE: A matches pos 0 (Green), G absent, R is in ALERT (Yellow), first E is in ALERT (Yellow), second E is absent
  game_evaluate_guess("AGREE", "ALERT", status);
  assert(status[0] == TILE_CORRECT);
  assert(status[1] == TILE_ABSENT);
  assert(status[2] == TILE_PRESENT);
  assert(status[3] == TILE_PRESENT);
  assert(status[4] == TILE_ABSENT);

  // Test 4: Target SPEED, Guess ERASE
  // SPEED has S(0), P(1), E(2), E(3), D(4)
  // ERASE: E is in SPEED (Yellow), R absent, A absent, S is in SPEED (Yellow), E is in SPEED (Yellow)
  game_evaluate_guess("ERASE", "SPEED", status);
  assert(status[0] == TILE_PRESENT);
  assert(status[1] == TILE_ABSENT);
  assert(status[2] == TILE_ABSENT);
  assert(status[3] == TILE_PRESENT);
  assert(status[4] == TILE_PRESENT);

  printf("[PASS] test_two_pass_clue_evaluation\n");
}

static void test_typing_and_backspace(void) {
  GameState game;
  game_init(&game);
  game_new_round(&game);

  game_handle_action(&game, ACT_LETTER, 'a');
  game_handle_action(&game, ACT_LETTER, 'P');
  game_handle_action(&game, ACT_LETTER, 'p');
  assert(game.current_letter_idx == 3);
  assert(strcmp(game.guesses[0].letters, "APP") == 0);

  // Backspace
  game_handle_action(&game, ACT_BACKSPACE, '\0');
  assert(game.current_letter_idx == 2);
  assert(strcmp(game.guesses[0].letters, "AP") == 0);

  // Type up to 5
  game_handle_action(&game, ACT_LETTER, 'P');
  game_handle_action(&game, ACT_LETTER, 'L');
  game_handle_action(&game, ACT_LETTER, 'E');
  assert(game.current_letter_idx == 5);
  assert(strcmp(game.guesses[0].letters, "APPLE") == 0);

  // 6th letter should be ignored
  game_handle_action(&game, ACT_LETTER, 'S');
  assert(game.current_letter_idx == 5);
  assert(strcmp(game.guesses[0].letters, "APPLE") == 0);

  printf("[PASS] test_typing_and_backspace\n");
}

static void test_invalid_guess_rejection(void) {
  GameState game;
  game_init(&game);
  game_new_round(&game);
  game_set_target_word(&game, "CRANE");

  // Incomplete guess (3 letters)
  game_handle_action(&game, ACT_LETTER, 'c');
  game_handle_action(&game, ACT_LETTER, 'a');
  game_handle_action(&game, ACT_LETTER, 't');
  assert(!game_submit_guess(&game));
  assert(game.current_attempt == 0);
  assert(strcmp(game.message, "NOT ENOUGH LETTERS") == 0);

  // Complete but nonsense word (not in dictionary)
  game_handle_action(&game, ACT_LETTER, 'z');
  game_handle_action(&game, ACT_LETTER, 'z');
  assert(!game_submit_guess(&game));
  assert(game.current_attempt == 0);
  assert(strcmp(game.message, "NOT IN WORD LIST") == 0);

  printf("[PASS] test_invalid_guess_rejection\n");
}

static void test_guess_progression_and_keyboard(void) {
  GameState game;
  game_init(&game);
  game_new_round(&game);
  game_set_target_word(&game, "CRANE");

  // Type STARE
  const char *guess = "STARE";
  for (int i = 0; i < 5; i++) {
    game_handle_action(&game, ACT_LETTER, guess[i]);
  }
  assert(game_submit_guess(&game));
  assert(game.current_attempt == 1);
  assert(game.current_letter_idx == 0);

  // In STARE vs CRANE:
  // S: Absent, T: Absent, A: Correct, R: Present, E: Correct
  assert(game.key_status['S' - 'A'] == KEY_ABSENT);
  assert(game.key_status['T' - 'A'] == KEY_ABSENT);
  assert(game.key_status['A' - 'A'] == KEY_CORRECT);
  assert(game.key_status['R' - 'A'] == KEY_PRESENT);
  assert(game.key_status['E' - 'A'] == KEY_CORRECT);

  printf("[PASS] test_guess_progression_and_keyboard\n");
}

static void test_win_condition(void) {
  GameState game;
  game_init(&game);
  game_new_round(&game);
  game_set_target_word(&game, "CRANE");
  int initial_wins = game.games_won;

  const char *win_guess = "CRANE";
  for (int i = 0; i < 5; i++) {
    game_handle_action(&game, ACT_LETTER, win_guess[i]);
  }
  assert(game_submit_guess(&game));
  assert(game.state == STATE_WON);
  assert(game.games_won == initial_wins + 1);
  assert(game.current_streak >= 1);
  assert(game.guess_distribution[0] >= 1);

  printf("[PASS] test_win_condition\n");
}

static void test_loss_condition(void) {
  GameState game;
  game_init(&game);
  game_new_round(&game);
  game_set_target_word(&game, "ROBOT");

  // Submit 6 wrong words
  const char *words[6] = {"STARE", "SLUMP", "CHALK", "DINGY", "FIGHT", "WHELP"};
  for (int w = 0; w < 6; w++) {
    for (int i = 0; i < 5; i++) {
      game_handle_action(&game, ACT_LETTER, words[w][i]);
    }
    game_submit_guess(&game);
  }

  assert(game.state == STATE_LOST);
  assert(game.current_streak == 0);
  assert(strstr(game.message, "ROBOT") != NULL);

  printf("[PASS] test_loss_condition\n");
}

static void test_auto_save_and_resume(void) {
  game_delete_save();
  GameState g1;
  game_init(&g1);
  game_new_round(&g1);
  game_set_target_word(&g1, "TIGER");

  // Make 1 valid guess
  const char *guess = "STARE";
  for (int i = 0; i < 5; i++) {
    game_handle_action(&g1, ACT_LETTER, guess[i]);
  }
  game_submit_guess(&g1);

  // Type partial 2nd guess
  game_handle_action(&g1, ACT_LETTER, 'G');
  game_handle_action(&g1, ACT_LETTER, 'R');
  game_save(&g1);

  // Load in g2
  GameState g2;
  game_init(&g2);

  assert(strcmp(g2.target_word, "TIGER") == 0);
  assert(g2.current_attempt == 1);
  assert(g2.current_letter_idx == 2);
  assert(strcmp(g2.guesses[0].letters, "STARE") == 0);
  assert(g2.guesses[1].letters[0] == 'G');
  assert(g2.guesses[1].letters[1] == 'R');
  assert(g2.state == STATE_PLAYING);

  game_delete_save();
  printf("[PASS] test_auto_save_and_resume\n");
}

int main(void) {
  srand(42);
  printf("Starting Wordle Logic & Dictionary Tests...\n");
  test_initialization_and_dictionary();
  test_two_pass_clue_evaluation();
  test_typing_and_backspace();
  test_invalid_guess_rejection();
  test_guess_progression_and_keyboard();
  test_win_condition();
  test_loss_condition();
  test_auto_save_and_resume();
  printf("All Wordle tests PASSED successfully!\n");
  return 0;
}
