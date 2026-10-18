#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "game.h"
#include "render.h"
#include "scores.h"

static void test_deck_generation_and_uniqueness(void) {
  GameState g;
  game_init(&g, 42);

  int suit_counts[4] = { 0 };
  int rank_counts[14] = { 0 };
  int total = 0;

  // Count tableau
  for (int col = 0; col < NUM_TABLEAU; col++) {
    for (int k = 0; k < g.tableau[col].count; k++) {
      Card c = g.tableau[col].cards[k];
      suit_counts[c.suit]++;
      rank_counts[c.rank]++;
      total++;
    }
  }

  // Count stock
  for (int k = 0; k < g.stock.count; k++) {
    Card c = g.stock.cards[k];
    suit_counts[c.suit]++;
    rank_counts[c.rank]++;
    total++;
  }

  assert(total == 52);
  for (int s = 0; s < 4; s++) {
    assert(suit_counts[s] == 13);
  }
  for (int r = 1; r <= 13; r++) {
    assert(rank_counts[r] == 4);
  }

  printf("[PASS] test_deck_generation_and_uniqueness\n");
}

static void test_initial_deal_layout(void) {
  GameState g;
  game_init(&g, 12345);

  int expected_counts[NUM_TABLEAU] = { 1, 2, 3, 4, 5, 6, 7 };
  int tableau_total = 0;

  for (int col = 0; col < NUM_TABLEAU; col++) {
    assert(g.tableau[col].count == expected_counts[col]);
    tableau_total += g.tableau[col].count;

    // Check hidden cards
    for (int k = 0; k < g.tableau[col].count - 1; k++) {
      assert(!g.tableau[col].cards[k].face_up);
    }
    // Top card must be face up
    assert(g.tableau[col].cards[g.tableau[col].count - 1].face_up);
  }

  assert(tableau_total == 28);
  assert(g.stock.count == 24);
  for (int k = 0; k < g.stock.count; k++) {
    assert(!g.stock.cards[k].face_up);
  }

  assert(g.waste.count == 0);
  for (int f = 0; f < NUM_FOUNDATIONS; f++) {
    assert(g.foundations[f].count == 0);
  }

  assert(g.score == 0);
  assert(g.moves == 0);
  assert(g.status == STATE_PLAYING);

  printf("[PASS] test_initial_deal_layout\n");
}

static void test_draw_stock_and_recycle(void) {
  GameState g;
  game_init(&g, 999);

  // Draw 24 cards
  for (int i = 0; i < 24; i++) {
    assert(g.stock.count == 24 - i);
    assert(g.waste.count == i);
    bool ok = game_draw_stock(&g);
    assert(ok);
    assert(g.waste.cards[g.waste.count - 1].face_up);
  }

  assert(g.stock.count == 0);
  assert(g.waste.count == 24);

  // Recycle waste back to stock
  g.score = 50; // Give some points to test deduction
  bool recycled = game_draw_stock(&g);
  assert(recycled);
  assert(g.stock.count == 24);
  assert(g.waste.count == 0);
  assert(g.score == 30); // 50 - 20 = 30
  for (int k = 0; k < g.stock.count; k++) {
    assert(!g.stock.cards[k].face_up);
  }

  printf("[PASS] test_draw_stock_and_recycle\n");
}

static void test_tableau_rules_and_alternating_colors(void) {
  Card red_8 = { SUIT_HEARTS, RANK_8, true };
  Card black_9 = { SUIT_SPADES, RANK_9, true };
  Card red_9 = { SUIT_DIAMONDS, RANK_9, true };
  Card black_10 = { SUIT_CLUBS, RANK_10, true };
  Card black_king = { SUIT_SPADES, RANK_K, true };
  Card red_queen = { SUIT_HEARTS, RANK_Q, true };

  // Red 8 on Black 9 -> valid
  assert(card_can_stack_tableau(red_8, black_9));

  // Red 8 on Red 9 -> invalid (same color)
  assert(!card_can_stack_tableau(red_8, red_9));

  // Red 8 on Black 10 -> invalid (rank gap 2)
  assert(!card_can_stack_tableau(red_8, black_10));

  // Face-down card cannot be stacked on
  Card face_down_9 = { SUIT_SPADES, RANK_9, false };
  assert(!card_can_stack_tableau(red_8, face_down_9));

  // Test empty tableau placement
  GameState g;
  game_init(&g, 100);
  g.tableau[0].count = 0; // Empty col 0

  g.waste.count = 1;
  g.waste.cards[0] = red_queen;
  // Queen on empty column -> invalid
  bool ok = game_move(&g, PILE_WASTE, 0, PILE_TABLEAU_1);
  assert(!ok);
  assert(g.tableau[0].count == 0);

  // King on empty column -> valid
  g.waste.cards[0] = black_king;
  ok = game_move(&g, PILE_WASTE, 0, PILE_TABLEAU_1);
  assert(ok);
  assert(g.tableau[0].count == 1);
  assert(g.tableau[0].cards[0].rank == RANK_K);

  printf("[PASS] test_tableau_rules_and_alternating_colors\n");
}

static void test_sub_stack_move_and_card_reveal(void) {
  GameState g;
  game_init(&g, 777);

  // Setup Column 1 with 1 face-down card and 1 face-up 8♥
  g.tableau[0].count = 2;
  g.tableau[0].cards[0] = (Card){ SUIT_CLUBS, RANK_3, false };
  g.tableau[0].cards[1] = (Card){ SUIT_HEARTS, RANK_8, true };

  // Setup Column 2 with 9♠ face up
  g.tableau[1].count = 1;
  g.tableau[1].cards[0] = (Card){ SUIT_SPADES, RANK_9, true };

  int score_before = g.score;
  bool ok = game_move(&g, PILE_TABLEAU_1, 1, PILE_TABLEAU_2);
  assert(ok);

  // Col 2 should now have 2 cards: 9♠ and 8♥
  assert(g.tableau[1].count == 2);
  assert(g.tableau[1].cards[1].rank == RANK_8);

  // Col 1 should have 1 card, and it should be automatically revealed!
  assert(g.tableau[0].count == 1);
  assert(g.tableau[0].cards[0].face_up);
  assert(g.score == score_before + 5); // +5 for reveal

  printf("[PASS] test_sub_stack_move_and_card_reveal\n");
}

static void test_foundation_rules_and_auto_move(void) {
  GameState g;
  game_init(&g, 444);

  Card ace_spades = { SUIT_SPADES, RANK_A, true };
  Card two_spades = { SUIT_SPADES, RANK_2, true };
  Card two_hearts = { SUIT_HEARTS, RANK_2, true };
  Card three_spades = { SUIT_SPADES, RANK_3, true };

  // Ace on empty foundation -> valid
  assert(card_can_stack_foundation(ace_spades, &g.foundations[0]));
  // 2 on empty foundation -> invalid
  assert(!card_can_stack_foundation(two_spades, &g.foundations[0]));

  g.foundations[0].cards[0] = ace_spades;
  g.foundations[0].count = 1;

  // 2 of spades on Ace of spades -> valid
  assert(card_can_stack_foundation(two_spades, &g.foundations[0]));
  // 2 of hearts on Ace of spades -> invalid (wrong suit)
  assert(!card_can_stack_foundation(two_hearts, &g.foundations[0]));
  // 3 of spades on Ace of spades -> invalid (rank gap)
  assert(!card_can_stack_foundation(three_spades, &g.foundations[0]));

  // Test game_auto_move
  g.waste.count = 1;
  g.waste.cards[0] = two_spades;

  bool auto_ok = game_auto_move(&g);
  assert(auto_ok);
  assert(g.foundations[0].count == 2);
  assert(g.foundations[0].cards[1].rank == RANK_2);
  assert(g.waste.count == 0);

  printf("[PASS] test_foundation_rules_and_auto_move\n");
}

static void test_undo_stack(void) {
  GameState g;
  game_init(&g, 555);

  int initial_stock = g.stock.count;
  int initial_waste = g.waste.count;

  // Draw card
  game_draw_stock(&g);
  assert(g.stock.count == initial_stock - 1);
  assert(g.waste.count == initial_waste + 1);

  // Undo draw
  bool undo_ok = game_undo(&g);
  assert(undo_ok);
  assert(g.stock.count == initial_stock);
  assert(g.waste.count == initial_waste);

  // Undo when stack empty -> returns false
  assert(!game_undo(&g));

  printf("[PASS] test_undo_stack\n");
}

static void test_win_condition_detection(void) {
  GameState g;
  game_init(&g, 123);

  for (int f = 0; f < NUM_FOUNDATIONS; f++) {
    g.foundations[f].count = 13;
    for (int r = 1; r <= 13; r++) {
      g.foundations[f].cards[r - 1] = (Card){ (Suit)f, (Rank)r, true };
    }
  }

  assert(g.status == STATE_PLAYING);
  game_check_win(&g);
  assert(g.status == STATE_WON);
  assert(g.won_celebration);

  printf("[PASS] test_win_condition_detection\n");
}

static void test_score_persistence(void) {
  int cur_score = arcade_score_get("solitaire", "high_score");
  arcade_score_set("solitaire", "high_score", 9999);
  assert(arcade_score_get("solitaire", "high_score") == 9999);

  // Restore original
  arcade_score_set("solitaire", "high_score", cur_score);
  assert(arcade_score_get("solitaire", "high_score") == cur_score);

  printf("[PASS] test_score_persistence\n");
}

static void test_render_frame_and_mouse_hit(void) {
  GameState g;
  game_init(&g, 777);

  // Test mouse coordinate decoding
  int card_idx = -1;
  assert(render_get_pile_at_pos(5, 5, &card_idx) == PILE_STOCK);
  assert(render_get_pile_at_pos(15, 5, &card_idx) == PILE_WASTE);
  assert(render_get_pile_at_pos(35, 5, &card_idx) == PILE_FOUNDATION_1);
  assert(render_get_pile_at_pos(45, 5, &card_idx) == PILE_FOUNDATION_2);
  assert(render_get_pile_at_pos(55, 5, &card_idx) == PILE_FOUNDATION_3);
  assert(render_get_pile_at_pos(65, 5, &card_idx) == PILE_FOUNDATION_4);

  assert(render_get_pile_at_pos(5, 10, &card_idx) == PILE_TABLEAU_1);
  assert(card_idx == 1);
  assert(render_get_pile_at_pos(65, 10, &card_idx) == PILE_TABLEAU_7);
  assert(render_get_pile_at_pos(0, 0, &card_idx) == PILE_NONE);

  // Verify render_game memory safety across states
  volatile int canary = 0x5A5A5A5A;
  render_game(&g);

  g.status = STATE_WON;
  render_game(&g);

  assert(canary == 0x5A5A5A5A);
  printf("[PASS] test_render_frame_and_mouse_hit\n");
}

int main(void) {
  printf("Running Solitaire unit tests...\n\n");

  test_deck_generation_and_uniqueness();
  test_initial_deal_layout();
  test_draw_stock_and_recycle();
  test_tableau_rules_and_alternating_colors();
  test_sub_stack_move_and_card_reveal();
  test_foundation_rules_and_auto_move();
  test_undo_stack();
  test_win_condition_detection();
  test_score_persistence();
  test_render_frame_and_mouse_hit();

  printf("\nAll Solitaire unit tests PASSED successfully!\n");
  return 0;
}

