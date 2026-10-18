#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scores.h"

static const char *RANK_STRINGS[] = {
  "", "A", "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K"
};

const char *rank_name(Rank r) {
  if (r >= RANK_A && r <= RANK_K) {
    return RANK_STRINGS[r];
  }
  return "?";
}

const char *suit_symbol(Suit s) {
  switch (s) {
    case SUIT_SPADES:   return "♠";
    case SUIT_HEARTS:   return "♥";
    case SUIT_DIAMONDS: return "♦";
    case SUIT_CLUBS:    return "♣";
    default:            return "?";
  }
}

bool card_is_red(Card c) {
  return (c.suit == SUIT_HEARTS || c.suit == SUIT_DIAMONDS);
}

bool card_is_black(Card c) {
  return (c.suit == SUIT_SPADES || c.suit == SUIT_CLUBS);
}

bool card_can_stack_tableau(Card card, Card target_top) {
  if (!target_top.face_up) return false;
  if (card.rank + 1 != target_top.rank) return false;
  return (card_is_red(card) != card_is_red(target_top));
}

bool card_can_stack_foundation(Card card, const CardPile *found) {
  if (found->count == 0) {
    return (card.rank == RANK_A);
  }
  Card top = found->cards[found->count - 1];
  return (card.suit == top.suit && card.rank == top.rank + 1);
}

static CardPile *get_pile_ptr(GameState *game, PileType pile) {
  if (pile == PILE_STOCK) return &game->stock;
  if (pile == PILE_WASTE) return &game->waste;
  if (pile >= PILE_FOUNDATION_1 && pile <= PILE_FOUNDATION_4) {
    return &game->foundations[pile - PILE_FOUNDATION_1];
  }
  if (pile >= PILE_TABLEAU_1 && pile <= PILE_TABLEAU_7) {
    return &game->tableau[pile - PILE_TABLEAU_1];
  }
  return NULL;
}

static const CardPile *get_pile_ptr_const(const GameState *game, PileType pile) {
  if (pile == PILE_STOCK) return &game->stock;
  if (pile == PILE_WASTE) return &game->waste;
  if (pile >= PILE_FOUNDATION_1 && pile <= PILE_FOUNDATION_4) {
    return &game->foundations[pile - PILE_FOUNDATION_1];
  }
  if (pile >= PILE_TABLEAU_1 && pile <= PILE_TABLEAU_7) {
    return &game->tableau[pile - PILE_TABLEAU_1];
  }
  return NULL;
}

static void save_undo(GameState *game) {
  if (game->undo_count >= MAX_UNDO_STEPS) {
    // Shift left by 1 to make room
    memmove(&game->undo_stack[0], &game->undo_stack[1], sizeof(GameUndoState) * (MAX_UNDO_STEPS - 1));
    game->undo_count = MAX_UNDO_STEPS - 1;
  }
  GameUndoState *u = &game->undo_stack[game->undo_count++];
  u->stock_count = game->stock.count;
  memcpy(u->stock, game->stock.cards, sizeof(Card) * game->stock.count);

  u->waste_count = game->waste.count;
  memcpy(u->waste, game->waste.cards, sizeof(Card) * game->waste.count);

  for (int f = 0; f < NUM_FOUNDATIONS; f++) {
    u->foundation_count[f] = game->foundations[f].count;
    memcpy(u->foundations[f], game->foundations[f].cards, sizeof(Card) * game->foundations[f].count);
  }

  for (int t = 0; t < NUM_TABLEAU; t++) {
    u->tableau_count[t] = game->tableau[t].count;
    memcpy(u->tableau[t], game->tableau[t].cards, sizeof(Card) * game->tableau[t].count);
  }

  u->score = game->score;
  u->moves = game->moves;
}

bool game_undo(GameState *game) {
  if (game->undo_count <= 0) {
    snprintf(game->status_msg, sizeof(game->status_msg), "No moves to undo");
    return false;
  }

  game->undo_count--;
  const GameUndoState *u = &game->undo_stack[game->undo_count];

  game->stock.count = u->stock_count;
  memcpy(game->stock.cards, u->stock, sizeof(Card) * u->stock_count);

  game->waste.count = u->waste_count;
  memcpy(game->waste.cards, u->waste, sizeof(Card) * u->waste_count);

  for (int f = 0; f < NUM_FOUNDATIONS; f++) {
    game->foundations[f].count = u->foundation_count[f];
    memcpy(game->foundations[f].cards, u->foundations[f], sizeof(Card) * u->foundation_count[f]);
  }

  for (int t = 0; t < NUM_TABLEAU; t++) {
    game->tableau[t].count = u->tableau_count[t];
    memcpy(game->tableau[t].cards, u->tableau[t], sizeof(Card) * u->tableau_count[t]);
  }

  game->score = u->score;
  game->moves = u->moves;
  game->selected_pile = PILE_NONE;
  game->selected_card_idx = -1;
  snprintf(game->status_msg, sizeof(game->status_msg), "Move undone (undo remaining: %d)", game->undo_count);
  return true;
}

void game_init(GameState *game, unsigned int seed) {
  memset(game, 0, sizeof(GameState));
  game->seed = seed;
  srand(seed);

  // Load scores
  game->high_score = arcade_score_get("solitaire", "high_score");
  game->games_won = arcade_score_get("solitaire", "games_won");
  game->games_played = arcade_score_get("solitaire", "games_played");
  game->games_played++;
  game_save_scores(game);

  game_restart(game);
}

void game_restart(GameState *game) {
  // Preserve scores & seed
  int high = game->high_score;
  int won = game->games_won;
  int played = game->games_played;
  unsigned int seed = game->seed;

  memset(game, 0, sizeof(GameState));
  game->high_score = high;
  game->games_won = won;
  game->games_played = played;
  game->seed = seed;
  srand(seed);

  // Generate 52 cards
  Card deck[TOTAL_CARDS];
  int idx = 0;
  for (int s = 0; s < 4; s++) {
    for (int r = 1; r <= 13; r++) {
      deck[idx].suit = (Suit)s;
      deck[idx].rank = (Rank)r;
      deck[idx].face_up = false;
      idx++;
    }
  }

  // Fisher-Yates shuffle
  for (int i = TOTAL_CARDS - 1; i > 0; i--) {
    int j = rand() % (i + 1);
    Card tmp = deck[i];
    deck[i] = deck[j];
    deck[j] = tmp;
  }

  // Deal tableau: col i gets (i + 1) cards
  int card_ptr = 0;
  for (int col = 0; col < NUM_TABLEAU; col++) {
    int count = col + 1;
    game->tableau[col].count = count;
    for (int k = 0; k < count; k++) {
      game->tableau[col].cards[k] = deck[card_ptr++];
      game->tableau[col].cards[k].face_up = (k == count - 1);
    }
  }

  // Remaining 24 cards go into stock
  game->stock.count = TOTAL_CARDS - card_ptr;
  for (int k = 0; k < game->stock.count; k++) {
    game->stock.cards[k] = deck[card_ptr++];
    game->stock.cards[k].face_up = false;
  }

  game->waste.count = 0;
  for (int f = 0; f < NUM_FOUNDATIONS; f++) {
    game->foundations[f].count = 0;
  }

  game->selected_pile = PILE_NONE;
  game->selected_card_idx = -1;
  game->cursor_pile = PILE_TABLEAU_1;
  game->cursor_card_idx = 0;
  game->status = STATE_PLAYING;
  game->score = 0;
  game->moves = 0;
  game->undo_count = 0;

  snprintf(game->status_msg, sizeof(game->status_msg), "Game dealt!");
}

bool game_draw_stock(GameState *game) {
  if (game->stock.count > 0) {
    save_undo(game);
    Card c = game->stock.cards[--game->stock.count];
    c.face_up = true;
    game->waste.cards[game->waste.count++] = c;
    game->moves++;
    snprintf(game->status_msg, sizeof(game->status_msg), "Drew %s%s from Stock", rank_name(c.rank), suit_symbol(c.suit));
    return true;
  }

  if (game->waste.count > 0) {
    save_undo(game);
    // Recycle waste in reverse order
    for (int i = game->waste.count - 1; i >= 0; i--) {
      Card c = game->waste.cards[i];
      c.face_up = false;
      game->stock.cards[game->stock.count++] = c;
    }
    game->waste.count = 0;
    game->score = (game->score >= 20) ? (game->score - 20) : 0;
    game->moves++;
    snprintf(game->status_msg, sizeof(game->status_msg), "Waste recycled into Stock (-20 pts)");
    return true;
  }

  snprintf(game->status_msg, sizeof(game->status_msg), "Stock and Waste are both empty");
  return false;
}

bool game_move(GameState *game, PileType src, int card_idx, PileType dst) {
  if (src == dst || src == PILE_NONE || dst == PILE_NONE) {
    return false;
  }

  CardPile *src_pile = get_pile_ptr(game, src);
  CardPile *dst_pile = get_pile_ptr(game, dst);
  if (!src_pile || !dst_pile || src_pile->count == 0) {
    return false;
  }

  // Handle Waste -> Foundation
  if (src == PILE_WASTE && dst >= PILE_FOUNDATION_1 && dst <= PILE_FOUNDATION_4) {
    Card c = src_pile->cards[src_pile->count - 1];
    if (card_can_stack_foundation(c, dst_pile)) {
      save_undo(game);
      src_pile->count--;
      dst_pile->cards[dst_pile->count++] = c;
      game->score += 10;
      game->moves++;
      snprintf(game->status_msg, sizeof(game->status_msg), "Moved %s%s to Foundation", rank_name(c.rank), suit_symbol(c.suit));
      game_check_win(game);
      return true;
    }
    snprintf(game->status_msg, sizeof(game->status_msg), "Cannot place %s%s on Foundation", rank_name(c.rank), suit_symbol(c.suit));
    return false;
  }

  // Handle Waste -> Tableau
  if (src == PILE_WASTE && dst >= PILE_TABLEAU_1 && dst <= PILE_TABLEAU_7) {
    Card c = src_pile->cards[src_pile->count - 1];
    bool valid = false;
    if (dst_pile->count == 0) {
      valid = (c.rank == RANK_K);
    } else {
      Card top = dst_pile->cards[dst_pile->count - 1];
      valid = card_can_stack_tableau(c, top);
    }

    if (valid) {
      save_undo(game);
      src_pile->count--;
      dst_pile->cards[dst_pile->count++] = c;
      game->score += 5;
      game->moves++;
      int col_num = dst - PILE_TABLEAU_1 + 1;
      snprintf(game->status_msg, sizeof(game->status_msg), "Moved %s%s to Column %d", rank_name(c.rank), suit_symbol(c.suit), col_num);
      return true;
    }
    snprintf(game->status_msg, sizeof(game->status_msg), "Illegal move to Column %d", dst - PILE_TABLEAU_1 + 1);
    return false;
  }

  // Handle Tableau -> Foundation
  if (src >= PILE_TABLEAU_1 && src <= PILE_TABLEAU_7 && dst >= PILE_FOUNDATION_1 && dst <= PILE_FOUNDATION_4) {
    // Only top card can move to Foundation
    Card c = src_pile->cards[src_pile->count - 1];
    if (card_can_stack_foundation(c, dst_pile)) {
      save_undo(game);
      src_pile->count--;
      dst_pile->cards[dst_pile->count++] = c;
      game->score += 10;
      game->moves++;

      // Reveal exposed card if hidden
      if (src_pile->count > 0 && !src_pile->cards[src_pile->count - 1].face_up) {
        src_pile->cards[src_pile->count - 1].face_up = true;
        game->score += 5;
      }

      snprintf(game->status_msg, sizeof(game->status_msg), "Moved %s%s to Foundation (+10)", rank_name(c.rank), suit_symbol(c.suit));
      game_check_win(game);
      return true;
    }
    snprintf(game->status_msg, sizeof(game->status_msg), "Cannot place %s%s on Foundation", rank_name(c.rank), suit_symbol(c.suit));
    return false;
  }

  // Handle Tableau -> Tableau
  if (src >= PILE_TABLEAU_1 && src <= PILE_TABLEAU_7 && dst >= PILE_TABLEAU_1 && dst <= PILE_TABLEAU_7) {
    if (card_idx < 0 || card_idx >= src_pile->count) {
      card_idx = src_pile->count - 1;
    }

    Card base_moving_card = src_pile->cards[card_idx];
    if (!base_moving_card.face_up) {
      snprintf(game->status_msg, sizeof(game->status_msg), "Cannot move face-down cards");
      return false;
    }

    bool valid = false;
    if (dst_pile->count == 0) {
      valid = (base_moving_card.rank == RANK_K);
    } else {
      Card dst_top = dst_pile->cards[dst_pile->count - 1];
      valid = card_can_stack_tableau(base_moving_card, dst_top);
    }

    if (valid) {
      save_undo(game);
      int move_count = src_pile->count - card_idx;
      for (int k = 0; k < move_count; k++) {
        dst_pile->cards[dst_pile->count++] = src_pile->cards[card_idx + k];
      }
      src_pile->count = card_idx;

      // Uncover newly exposed card if face down
      if (src_pile->count > 0 && !src_pile->cards[src_pile->count - 1].face_up) {
        src_pile->cards[src_pile->count - 1].face_up = true;
        game->score += 5;
      }

      game->moves++;
      int src_col = src - PILE_TABLEAU_1 + 1;
      int dst_col = dst - PILE_TABLEAU_1 + 1;
      snprintf(game->status_msg, sizeof(game->status_msg), "Moved %d card(s) from Col %d to Col %d", move_count, src_col, dst_col);
      return true;
    }

    snprintf(game->status_msg, sizeof(game->status_msg), "Illegal move to Col %d", dst - PILE_TABLEAU_1 + 1);
    return false;
  }

  // Handle Foundation -> Tableau
  if (src >= PILE_FOUNDATION_1 && src <= PILE_FOUNDATION_4 && dst >= PILE_TABLEAU_1 && dst <= PILE_TABLEAU_7) {
    Card c = src_pile->cards[src_pile->count - 1];
    bool valid = false;
    if (dst_pile->count == 0) {
      valid = (c.rank == RANK_K);
    } else {
      Card top = dst_pile->cards[dst_pile->count - 1];
      valid = card_can_stack_tableau(c, top);
    }

    if (valid) {
      save_undo(game);
      src_pile->count--;
      dst_pile->cards[dst_pile->count++] = c;
      game->score = (game->score >= 15) ? (game->score - 15) : 0;
      game->moves++;
      int col_num = dst - PILE_TABLEAU_1 + 1;
      snprintf(game->status_msg, sizeof(game->status_msg), "Moved %s%s to Col %d (-15 pts)", rank_name(c.rank), suit_symbol(c.suit), col_num);
      return true;
    }
  }

  return false;
}

bool game_auto_foundation_for_pile(GameState *game, PileType src) {
  const CardPile *pile = get_pile_ptr_const(game, src);
  if (!pile || pile->count == 0) return false;

  Card c = pile->cards[pile->count - 1];
  if (!c.face_up) return false;

  // Try each foundation
  for (int f = 0; f < NUM_FOUNDATIONS; f++) {
    PileType dst = (PileType)(PILE_FOUNDATION_1 + f);
    if (card_can_stack_foundation(c, &game->foundations[f])) {
      return game_move(game, src, pile->count - 1, dst);
    }
  }
  return false;
}

bool game_auto_move(GameState *game) {
  // 1. Check Waste
  if (game_auto_foundation_for_pile(game, PILE_WASTE)) {
    return true;
  }

  // 2. Check all Tableau columns from 1 to 7
  for (int col = 0; col < NUM_TABLEAU; col++) {
    PileType src = (PileType)(PILE_TABLEAU_1 + col);
    if (game_auto_foundation_for_pile(game, src)) {
      return true;
    }
  }

  snprintf(game->status_msg, sizeof(game->status_msg), "No eligible cards for Foundation");
  return false;
}

bool game_is_all_exposed(const GameState *game) {
  if (game->stock.count > 0 || game->waste.count > 0) {
    return false;
  }
  for (int col = 0; col < NUM_TABLEAU; col++) {
    for (int k = 0; k < game->tableau[col].count; k++) {
      if (!game->tableau[col].cards[k].face_up) {
        return false;
      }
    }
  }
  return true;
}

void game_check_win(GameState *game) {
  int total = 0;
  for (int f = 0; f < NUM_FOUNDATIONS; f++) {
    total += game->foundations[f].count;
  }

  if (total == TOTAL_CARDS) {
    game->status = STATE_WON;
    game->won_celebration = true;
    game->games_won++;
    if (game->score > game->high_score) {
      game->high_score = game->score;
    }
    game_save_scores(game);
    snprintf(game->status_msg, sizeof(game->status_msg), "CONGRATULATIONS! You solved Klondike Solitaire!");
  }
}

void game_save_scores(const GameState *game) {
  arcade_score_set("solitaire", "high_score", game->high_score);
  arcade_score_set("solitaire", "games_won", game->games_won);
  arcade_score_set("solitaire", "games_played", game->games_played);
}

bool game_get_hint(const GameState *game, char *out_hint, size_t buf_size) {
  if (!out_hint || buf_size == 0) return false;

  // 1. Can waste go to foundation?
  if (game->waste.count > 0) {
    Card c = game->waste.cards[game->waste.count - 1];
    for (int f = 0; f < NUM_FOUNDATIONS; f++) {
      if (card_can_stack_foundation(c, &game->foundations[f])) {
        snprintf(out_hint, buf_size, "Move %s%s from Waste to Foundation", rank_name(c.rank), suit_symbol(c.suit));
        return true;
      }
    }
  }

  // 2. Can tableau top card go to foundation?
  for (int col = 0; col < NUM_TABLEAU; col++) {
    if (game->tableau[col].count > 0) {
      Card c = game->tableau[col].cards[game->tableau[col].count - 1];
      if (c.face_up) {
        for (int f = 0; f < NUM_FOUNDATIONS; f++) {
          if (card_can_stack_foundation(c, &game->foundations[f])) {
            snprintf(out_hint, buf_size, "Move %s%s from Col %d to Foundation", rank_name(c.rank), suit_symbol(c.suit), col + 1);
            return true;
          }
        }
      }
    }
  }

  // 3. Can moving a tableau stack reveal a face-down card?
  for (int src_col = 0; src_col < NUM_TABLEAU; src_col++) {
    const CardPile *src = &game->tableau[src_col];
    // Find the first face-up card
    int first_face_up = -1;
    for (int k = 0; k < src->count; k++) {
      if (src->cards[k].face_up) {
        first_face_up = k;
        break;
      }
    }
    if (first_face_up > 0) { // Has face-down cards underneath!
      Card base = src->cards[first_face_up];
      for (int dst_col = 0; dst_col < NUM_TABLEAU; dst_col++) {
        if (src_col == dst_col) continue;
        const CardPile *dst = &game->tableau[dst_col];
        bool can_place = false;
        if (dst->count == 0) {
          can_place = (base.rank == RANK_K);
        } else {
          can_place = card_can_stack_tableau(base, dst->cards[dst->count - 1]);
        }
        if (can_place) {
          snprintf(out_hint, buf_size, "Move %s%s from Col %d to Col %d to uncover card",
                   rank_name(base.rank), suit_symbol(base.suit), src_col + 1, dst_col + 1);
          return true;
        }
      }
    }
  }

  // 4. Can waste card go to tableau?
  if (game->waste.count > 0) {
    Card c = game->waste.cards[game->waste.count - 1];
    for (int dst_col = 0; dst_col < NUM_TABLEAU; dst_col++) {
      const CardPile *dst = &game->tableau[dst_col];
      bool can_place = false;
      if (dst->count == 0) {
        can_place = (c.rank == RANK_K);
      } else {
        can_place = card_can_stack_tableau(c, dst->cards[dst->count - 1]);
      }
      if (can_place) {
        snprintf(out_hint, buf_size, "Move %s%s from Waste to Col %d",
                 rank_name(c.rank), suit_symbol(c.suit), dst_col + 1);
        return true;
      }
    }
  }

  // 5. Any other tableau to tableau move
  for (int src_col = 0; src_col < NUM_TABLEAU; src_col++) {
    const CardPile *src = &game->tableau[src_col];
    int first_face_up = -1;
    for (int k = 0; k < src->count; k++) {
      if (src->cards[k].face_up) {
        first_face_up = k;
        break;
      }
    }
    if (first_face_up >= 0) {
      Card base = src->cards[first_face_up];
      for (int dst_col = 0; dst_col < NUM_TABLEAU; dst_col++) {
        if (src_col == dst_col) continue;
        const CardPile *dst = &game->tableau[dst_col];
        // Don't just shift King between empty columns
        if (dst->count == 0 && first_face_up == 0) continue;

        bool can_place = false;
        if (dst->count == 0) {
          can_place = (base.rank == RANK_K);
        } else {
          can_place = card_can_stack_tableau(base, dst->cards[dst->count - 1]);
        }
        if (can_place) {
          snprintf(out_hint, buf_size, "Move %s%s from Col %d to Col %d",
                   rank_name(base.rank), suit_symbol(base.suit), src_col + 1, dst_col + 1);
          return true;
        }
      }
    }
  }

  // 6. Draw from Stock
  if (game->stock.count > 0) {
    snprintf(out_hint, buf_size, "Draw card from Stock [S]");
    return true;
  }

  // 7. Recycle Stock
  if (game->waste.count > 0) {
    snprintf(out_hint, buf_size, "Recycle Waste pile into Stock [S]");
    return true;
  }

  snprintf(out_hint, buf_size, "No more legal moves");
  return false;
}

void game_select_pile(GameState *game, PileType pile) {
  if (game->selected_pile == PILE_NONE) {
    // Attempting to select a source
    if (pile == PILE_STOCK) {
      // Direct action: drawing from stock
      game_draw_stock(game);
      return;
    }

    CardPile *p = get_pile_ptr(game, pile);
    if (!p || p->count == 0) {
      snprintf(game->status_msg, sizeof(game->status_msg), "Pile is empty");
      return;
    }

    if (pile == PILE_WASTE) {
      game->selected_pile = PILE_WASTE;
      game->selected_card_idx = p->count - 1;
      Card c = p->cards[p->count - 1];
      snprintf(game->status_msg, sizeof(game->status_msg), "Selected Waste %s%s. Choose destination [1-7] or [F]", rank_name(c.rank), suit_symbol(c.suit));
      return;
    }

    if (pile >= PILE_TABLEAU_1 && pile <= PILE_TABLEAU_7) {
      // Find the first face-up card by default
      int first_up = -1;
      for (int k = 0; k < p->count; k++) {
        if (p->cards[k].face_up) {
          first_up = k;
          break;
        }
      }
      if (first_up == -1) {
        snprintf(game->status_msg, sizeof(game->status_msg), "No face-up cards in this column");
        return;
      }
      game->selected_pile = pile;
      game->selected_card_idx = first_up;
      Card c = p->cards[first_up];
      int col_num = pile - PILE_TABLEAU_1 + 1;
      snprintf(game->status_msg, sizeof(game->status_msg), "Selected Col %d [%s%s]. Choose target [1-7] or [F]", col_num, rank_name(c.rank), suit_symbol(c.suit));
      return;
    }

    if (pile >= PILE_FOUNDATION_1 && pile <= PILE_FOUNDATION_4) {
      game->selected_pile = pile;
      game->selected_card_idx = p->count - 1;
      Card c = p->cards[p->count - 1];
      snprintf(game->status_msg, sizeof(game->status_msg), "Selected Foundation %s%s. Choose destination [1-7]", rank_name(c.rank), suit_symbol(c.suit));
      return;
    }
  } else {
    // A source is already selected!
    if (game->selected_pile == pile) {
      // Deselect
      game_clear_selection(game);
      snprintf(game->status_msg, sizeof(game->status_msg), "Selection cleared");
      return;
    }

    // Move from selected_pile to target pile!
    bool success = game_move(game, game->selected_pile, game->selected_card_idx, pile);
    if (success) {
      game_clear_selection(game);
    }
  }
}

void game_select_tableau_card(GameState *game, int tableau_idx, int card_idx) {
  if (tableau_idx < 0 || tableau_idx >= NUM_TABLEAU) return;
  PileType pile = (PileType)(PILE_TABLEAU_1 + tableau_idx);
  CardPile *p = &game->tableau[tableau_idx];

  if (game->selected_pile == PILE_NONE) {
    if (card_idx < 0 || card_idx >= p->count || !p->cards[card_idx].face_up) {
      return;
    }
    game->selected_pile = pile;
    game->selected_card_idx = card_idx;
    Card c = p->cards[card_idx];
    snprintf(game->status_msg, sizeof(game->status_msg), "Selected Col %d [%s%s]. Choose destination", tableau_idx + 1, rank_name(c.rank), suit_symbol(c.suit));
  } else {
    game_select_pile(game, pile);
  }
}

void game_clear_selection(GameState *game) {
  game->selected_pile = PILE_NONE;
  game->selected_card_idx = -1;
}

void game_nav_cursor(GameState *game, GameAction nav_act) {
  switch (nav_act) {
    case ACT_NAV_LEFT:
      if (game->cursor_pile == PILE_WASTE) {
        game->cursor_pile = PILE_STOCK;
      } else if (game->cursor_pile == PILE_FOUNDATION_1) {
        game->cursor_pile = PILE_WASTE;
      } else if (game->cursor_pile > PILE_FOUNDATION_1 && game->cursor_pile <= PILE_FOUNDATION_4) {
        game->cursor_pile = (PileType)(game->cursor_pile - 1);
      } else if (game->cursor_pile > PILE_TABLEAU_1 && game->cursor_pile <= PILE_TABLEAU_7) {
        game->cursor_pile = (PileType)(game->cursor_pile - 1);
        int col = game->cursor_pile - PILE_TABLEAU_1;
        game->cursor_card_idx = (game->tableau[col].count > 0) ? (game->tableau[col].count - 1) : 0;
      }
      break;

    case ACT_NAV_RIGHT:
      if (game->cursor_pile == PILE_STOCK) {
        game->cursor_pile = PILE_WASTE;
      } else if (game->cursor_pile == PILE_WASTE) {
        game->cursor_pile = PILE_FOUNDATION_1;
      } else if (game->cursor_pile >= PILE_FOUNDATION_1 && game->cursor_pile < PILE_FOUNDATION_4) {
        game->cursor_pile = (PileType)(game->cursor_pile + 1);
      } else if (game->cursor_pile >= PILE_TABLEAU_1 && game->cursor_pile < PILE_TABLEAU_7) {
        game->cursor_pile = (PileType)(game->cursor_pile + 1);
        int col = game->cursor_pile - PILE_TABLEAU_1;
        game->cursor_card_idx = (game->tableau[col].count > 0) ? (game->tableau[col].count - 1) : 0;
      }
      break;

    case ACT_NAV_UP:
      if (game->cursor_pile >= PILE_TABLEAU_1 && game->cursor_pile <= PILE_TABLEAU_7) {
        int col = game->cursor_pile - PILE_TABLEAU_1;
        // Move selection up through face up cards
        if (game->cursor_card_idx > 0 && game->tableau[col].cards[game->cursor_card_idx - 1].face_up) {
          game->cursor_card_idx--;
        } else {
          // Jump to top row
          if (col == 0) game->cursor_pile = PILE_STOCK;
          else if (col == 1) game->cursor_pile = PILE_WASTE;
          else if (col >= 2 && col <= 5) game->cursor_pile = (PileType)(PILE_FOUNDATION_1 + (col - 2));
          else game->cursor_pile = PILE_FOUNDATION_4;
        }
      }
      break;

    case ACT_NAV_DOWN:
      if (game->cursor_pile <= PILE_FOUNDATION_4) {
        // Jump from top row down to tableau
        if (game->cursor_pile == PILE_STOCK) game->cursor_pile = PILE_TABLEAU_1;
        else if (game->cursor_pile == PILE_WASTE) game->cursor_pile = PILE_TABLEAU_2;
        else {
          int f = game->cursor_pile - PILE_FOUNDATION_1;
          game->cursor_pile = (PileType)(PILE_TABLEAU_3 + f);
        }
        int col = game->cursor_pile - PILE_TABLEAU_1;
        game->cursor_card_idx = (game->tableau[col].count > 0) ? (game->tableau[col].count - 1) : 0;
      } else if (game->cursor_pile >= PILE_TABLEAU_1 && game->cursor_pile <= PILE_TABLEAU_7) {
        int col = game->cursor_pile - PILE_TABLEAU_1;
        if (game->cursor_card_idx + 1 < game->tableau[col].count) {
          game->cursor_card_idx++;
        }
      }
      break;

    default:
      break;
  }
}

