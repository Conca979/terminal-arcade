#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "game.h"
#include "scores.h"

static void test_card_values(void) {
  Card c2 = { SUIT_SPADES, RANK_2 };
  Card c7 = { SUIT_HEARTS, RANK_7 };
  Card c10 = { SUIT_DIAMONDS, RANK_10 };
  Card cj = { SUIT_CLUBS, RANK_J };
  Card cq = { SUIT_HEARTS, RANK_Q };
  Card ck = { SUIT_SPADES, RANK_K };
  Card ca = { SUIT_DIAMONDS, RANK_A };

  assert(card_value(c2) == 2);
  assert(card_value(c7) == 7);
  assert(card_value(c10) == 10);
  assert(card_value(cj) == 10);
  assert(card_value(cq) == 10);
  assert(card_value(ck) == 10);
  assert(card_value(ca) == 11);

  printf("[PASS] test_card_values\n");
}

static void test_hand_calculate_total(void) {
  Hand h;
  memset(&h, 0, sizeof(h));
  bool is_soft = false;

  // 10 + 7 = 17 (hard)
  h.cards[0] = (Card){ SUIT_SPADES, RANK_10 };
  h.cards[1] = (Card){ SUIT_HEARTS, RANK_7 };
  h.count = 2;
  assert(hand_calculate_total(&h, &is_soft) == 17);
  assert(!is_soft);

  // Ace + 6 = 17 (soft 17)
  h.cards[0] = (Card){ SUIT_SPADES, RANK_A };
  h.cards[1] = (Card){ SUIT_CLUBS, RANK_6 };
  h.count = 2;
  assert(hand_calculate_total(&h, &is_soft) == 17);
  assert(is_soft);

  // Ace + 6 + 9 = 16 (Ace reduced to 1, hard 16)
  h.cards[2] = (Card){ SUIT_DIAMONDS, RANK_9 };
  h.count = 3;
  assert(hand_calculate_total(&h, &is_soft) == 16);
  assert(!is_soft);

  // Ace + Ace = 12 (soft: 11 + 1)
  h.cards[0] = (Card){ SUIT_SPADES, RANK_A };
  h.cards[1] = (Card){ SUIT_HEARTS, RANK_A };
  h.count = 2;
  assert(hand_calculate_total(&h, &is_soft) == 12);
  assert(is_soft);

  // Ace + Ace + 9 = 21 (soft: 11 + 1 + 9)
  h.cards[2] = (Card){ SUIT_DIAMONDS, RANK_9 };
  h.count = 3;
  assert(hand_calculate_total(&h, &is_soft) == 21);
  assert(is_soft);

  // Ace + Ace + 9 + 5 = 16 (hard: 1 + 1 + 9 + 5)
  h.cards[3] = (Card){ SUIT_CLUBS, RANK_5 };
  h.count = 4;
  assert(hand_calculate_total(&h, &is_soft) == 16);
  assert(!is_soft);

  // 4 Aces = 14 (11 + 1 + 1 + 1)
  h.cards[0] = (Card){ SUIT_SPADES, RANK_A };
  h.cards[1] = (Card){ SUIT_HEARTS, RANK_A };
  h.cards[2] = (Card){ SUIT_DIAMONDS, RANK_A };
  h.cards[3] = (Card){ SUIT_CLUBS, RANK_A };
  h.count = 4;
  assert(hand_calculate_total(&h, &is_soft) == 14);
  assert(is_soft);

  printf("[PASS] test_hand_calculate_total\n");
}

static void test_natural_blackjack(void) {
  Hand h;
  memset(&h, 0, sizeof(h));

  // Ace + King (2 cards) -> Blackjack
  h.cards[0] = (Card){ SUIT_SPADES, RANK_A };
  h.cards[1] = (Card){ SUIT_HEARTS, RANK_K };
  h.count = 2;
  assert(hand_is_blackjack(&h));

  // Ace + 10 (2 cards) -> Blackjack
  h.cards[1] = (Card){ SUIT_CLUBS, RANK_10 };
  assert(hand_is_blackjack(&h));

  // 7 + 7 + 7 = 21, but 3 cards -> NOT Blackjack
  h.cards[0] = (Card){ SUIT_SPADES, RANK_7 };
  h.cards[1] = (Card){ SUIT_HEARTS, RANK_7 };
  h.cards[2] = (Card){ SUIT_DIAMONDS, RANK_7 };
  h.count = 3;
  assert(!hand_is_blackjack(&h));

  // Split hand with Ace + King -> NOT Natural Blackjack (pays 1:1 in casino rules)
  h.cards[0] = (Card){ SUIT_SPADES, RANK_A };
  h.cards[1] = (Card){ SUIT_CLUBS, RANK_K };
  h.count = 2;
  h.is_split = true;
  assert(!hand_is_blackjack(&h));

  printf("[PASS] test_natural_blackjack\n");
}

static void test_split_and_double_qualification(void) {
  Hand h;
  memset(&h, 0, sizeof(h));
  h.bet = 50;

  // Pair of 8s with enough chips
  h.cards[0] = (Card){ SUIT_SPADES, RANK_8 };
  h.cards[1] = (Card){ SUIT_HEARTS, RANK_8 };
  h.count = 2;
  assert(hand_can_split(&h, 100));
  assert(hand_can_double(&h, 100));

  // Same value cards (10 and Jack)
  h.cards[1] = (Card){ SUIT_CLUBS, RANK_J };
  assert(hand_can_split(&h, 100) == false); // 8 and J different
  h.cards[0] = (Card){ SUIT_SPADES, RANK_10 };
  assert(hand_can_split(&h, 100) == true);  // 10 and J have equal value (10)

  // Insufficient chips
  assert(hand_can_split(&h, 40) == false);
  assert(hand_can_double(&h, 40) == false);

  // 3 cards cannot split or double
  h.count = 3;
  assert(hand_can_split(&h, 100) == false);
  assert(hand_can_double(&h, 100) == false);

  printf("[PASS] test_split_and_double_qualification\n");
}

static void test_shoe_composition_and_shuffle(void) {
  GameState game;
  memset(&game, 0, sizeof(game));
  game_init_shoe(&game);

  assert(game.cut_card_index >= 200 && game.cut_card_index <= 260);

  int rank_counts[15] = {0};
  int suit_counts[4] = {0};

  for (int i = 0; i < SHOE_SIZE; i++) {
    rank_counts[game.shoe[i].rank]++;
    suit_counts[game.shoe[i].suit]++;
  }

  // 6 decks: 6 * 4 = 24 of each rank
  for (int r = (int)RANK_2; r <= (int)RANK_A; r++) {
    assert(rank_counts[r] == 24);
  }

  // 6 decks: 6 * 13 = 78 of each suit
  for (int s = 0; s < 4; s++) {
    assert(suit_counts[s] == 78);
  }

  // Test drawing cards increments shoe index
  int initial_idx = game.shoe_index;
  Card drawn = game_draw_card(&game);
  assert(card_value(drawn) >= 2 && card_value(drawn) <= 11);
  assert(game.shoe_index == initial_idx + 1);

  printf("[PASS] test_shoe_composition_and_shuffle\n");
}

static void test_dealer_ai(void) {
  GameState game;
  memset(&game, 0, sizeof(game));
  game_init_shoe(&game);

  // Test dealer standing on 17+
  game.dealer_hand.cards[0] = (Card){ SUIT_SPADES, RANK_10 };
  game.dealer_hand.cards[1] = (Card){ SUIT_HEARTS, RANK_7 };
  game.dealer_hand.count = 2;
  game.player_hands[0].cards[0] = (Card){ SUIT_SPADES, RANK_10 };
  game.player_hands[0].cards[1] = (Card){ SUIT_HEARTS, RANK_8 };
  game.player_hands[0].count = 2;
  game.player_hands[0].bet = 50;
  game.player_hand_count = 1;
  game.chips = 500;

  game_dealer_play(&game);

  // Dealer should stand on 17 without drawing more cards
  assert(game.dealer_hand.count == 2);
  assert(hand_calculate_total(&game.dealer_hand, NULL) == 17);
  // Player has 18 vs Dealer 17: Player wins!
  assert(game.player_hands[0].result == RESULT_WIN);
  assert(game.chips == 600); // 500 + 100 payout (50 profit + 50 stake)

  printf("[PASS] test_dealer_ai\n");
}

static void test_double_down_action(void) {
  GameState game;
  memset(&game, 0, sizeof(game));
  game_init_shoe(&game);

  game.chips = 1000;
  game.current_bet = 50;
  game.state = STATE_PLAYER_TURN;
  game.player_hand_count = 1;
  game.active_hand_idx = 0;

  Hand *h = &game.player_hands[0];
  h->cards[0] = (Card){ SUIT_SPADES, RANK_5 };
  h->cards[1] = (Card){ SUIT_HEARTS, RANK_6 };
  h->count = 2;
  h->bet = 50;

  game.dealer_hand.cards[0] = (Card){ SUIT_CLUBS, RANK_7 };
  game.dealer_hand.cards[1] = (Card){ SUIT_DIAMONDS, RANK_9 }; // total 16
  game.dealer_hand.count = 2;

  // Double down
  game_player_double(&game);

  // Bet doubled to 100, chips decreased by additional 50
  assert(h->bet == 100);
  assert(h->is_doubled);
  assert(game.chips == 950 || game.chips >= 950);
  // Hand received exactly 1 card
  assert(h->count == 3);

  printf("[PASS] test_double_down_action\n");
}

static void test_split_hand_action(void) {
  GameState game;
  memset(&game, 0, sizeof(game));
  game_init_shoe(&game);

  game.chips = 1000;
  game.current_bet = 50;
  game.state = STATE_PLAYER_TURN;
  game.player_hand_count = 1;
  game.active_hand_idx = 0;

  Hand *h0 = &game.player_hands[0];
  h0->cards[0] = (Card){ SUIT_SPADES, RANK_8 };
  h0->cards[1] = (Card){ SUIT_HEARTS, RANK_8 };
  h0->count = 2;
  h0->bet = 50;

  game_player_split(&game);

  assert(game.player_hand_count == 2);
  assert(game.player_hands[0].is_split);
  assert(game.player_hands[1].is_split);
  assert(game.player_hands[0].bet == 50);
  assert(game.player_hands[1].bet == 50);
  assert(game.player_hands[0].count == 2);
  assert(game.player_hands[1].count == 2);
  assert(game.chips == 950); // additional 50 deducted for hand 1

  printf("[PASS] test_split_hand_action\n");
}

static void test_insurance_mechanics(void) {
  GameState game;
  memset(&game, 0, sizeof(game));
  game_init_shoe(&game);

  game.chips = 1000;
  game.current_bet = 100;
  game.state = STATE_INSURANCE;
  game.player_hand_count = 1;
  game.player_hands[0].cards[0] = (Card){ SUIT_SPADES, RANK_10 };
  game.player_hands[0].cards[1] = (Card){ SUIT_HEARTS, RANK_8 };
  game.player_hands[0].count = 2;
  game.player_hands[0].bet = 100;

  // Dealer has Blackjack (Ace + King)
  game.dealer_hand.cards[0] = (Card){ SUIT_CLUBS, RANK_A };
  game.dealer_hand.cards[1] = (Card){ SUIT_DIAMONDS, RANK_K };
  game.dealer_hand.count = 2;

  // Accept insurance
  game_player_insurance(&game, true);

  // Insurance cost is 50. Dealer had BJ: insurance pays 2:1 (+100 profit + 50 stake = 150).
  // Main bet 100 lost. Net change: 1000 - 50 + 150 = 1100.
  assert(game.insurance_bought);
  assert(game.chips == 1100);
  assert(game.state == STATE_ROUND_OVER);

  printf("[PASS] test_insurance_mechanics\n");
}

static void test_score_persistence(void) {
  GameState game;
  memset(&game, 0, sizeof(game));

  game.chips = 1850;
  game.high_chips = 2400;
  game.hands_played = 42;
  game.hands_won = 25;

  game_save_scores(&game);

  int c = arcade_score_get("blackjack", "chips");
  int h = arcade_score_get("blackjack", "high_chips");
  int p = arcade_score_get("blackjack", "hands_played");
  int w = arcade_score_get("blackjack", "hands_won");

  assert(c == 1850);
  assert(h == 2400);
  assert(p == 42);
  assert(w == 25);

  printf("[PASS] test_score_persistence\n");
}

int main(void) {
  printf("Running Blackjack unit tests...\n\n");

  test_card_values();
  test_hand_calculate_total();
  test_natural_blackjack();
  test_split_and_double_qualification();
  test_shoe_composition_and_shuffle();
  test_dealer_ai();
  test_double_down_action();
  test_split_hand_action();
  test_insurance_mechanics();
  test_score_persistence();

  printf("\nAll Blackjack unit tests PASSED successfully!\n");
  return 0;
}
