#include "config.h"
#include "game.h"
#include "render.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_init_and_board_setup(void) {
  GameState g;
  game_init(&g, THEME_SUNSET);

  assert(g.current_theme == THEME_SUNSET);
  assert(g.holding_piece == -1);
  assert(g.moves_count == 0);
  assert(g.snapped_count == 0);
  assert(!g.is_solved);

  for (int r = 0; r < GRID_SIZE; r++) {
    for (int c = 0; c < GRID_SIZE; c++) {
      assert(g.board[r][c] == -1);
    }
  }

  for (int i = 0; i < TOTAL_PIECES; i++) {
    assert(g.tray_present[i] == true);
  }

  printf("[PASS] test_init_and_board_setup\n");
}

static void test_pick_and_place_snap(void) {
  GameState g;
  game_init(&g, THEME_SUNSET);

  /* Pick piece 0 (home slot is 0, 0) */
  assert(game_pick_from_tray(&g, 0));
  assert(g.holding_piece == 0);
  assert(!g.tray_present[0]);

  /* Place on board at wrong slot (0, 1) */
  assert(game_place_on_board(&g, 0, 1));
  assert(g.holding_piece == -1);
  assert(g.board[0][1] == 0);
  assert(g.moves_count == 1);
  assert(g.snapped_count == 0); /* Not at home slot */

  /* Pick piece 0 back up from (0, 1) */
  assert(game_place_on_board(&g, 0, 1));
  assert(g.holding_piece == 0);
  assert(g.board[0][1] == -1);

  /* Place on board at correct slot (0, 0) */
  assert(game_place_on_board(&g, 0, 0));
  assert(g.board[0][0] == 0);
  assert(g.snapped_count == 1); /* Snapped! */

  printf("[PASS] test_pick_and_place_snap\n");
}

static void test_swap_pieces(void) {
  GameState g;
  game_init(&g, THEME_SUNSET);

  /* Place piece 1 at (0, 0) */
  assert(game_pick_from_tray(&g, 1));
  assert(game_place_on_board(&g, 0, 0));
  assert(g.board[0][0] == 1);
  assert(g.holding_piece == -1);

  /* Now pick piece 0 and place at (0, 0) -> should swap! */
  assert(game_pick_from_tray(&g, 0));
  assert(game_place_on_board(&g, 0, 0));
  assert(g.board[0][0] == 0); /* Piece 0 placed */
  assert(g.holding_piece == 1); /* Piece 1 now in hand */
  assert(g.snapped_count == 1);

  printf("[PASS] test_swap_pieces\n");
}

static void test_return_to_tray(void) {
  GameState g;
  game_init(&g, THEME_SUNSET);

  /* Pick piece 3, then return holding to tray */
  assert(game_pick_from_tray(&g, 3));
  assert(!g.tray_present[3]);
  assert(game_return_holding_to_tray(&g));
  assert(g.holding_piece == -1);
  assert(g.tray_present[3]);

  /* Place piece 5 on board at (1, 1), then return from slot to tray */
  assert(game_pick_from_tray(&g, 5));
  assert(game_place_on_board(&g, 1, 1));
  assert(g.board[1][1] == 5);
  assert(g.snapped_count == 1);

  assert(game_return_slot_to_tray(&g, 1, 1));
  assert(g.board[1][1] == -1);
  assert(g.tray_present[5]);
  assert(g.snapped_count == 0);

  printf("[PASS] test_return_to_tray\n");
}

static void test_win_condition(void) {
  GameState g;
  game_init(&g, THEME_SUNSET);

  int initial_solved = g.puzzles_solved;

  /* Place all 16 pieces into their correct home slots */
  for (int r = 0; r < GRID_SIZE; r++) {
    for (int c = 0; c < GRID_SIZE; c++) {
      int p = r * GRID_SIZE + c;
      assert(game_pick_from_tray(&g, p));
      assert(game_place_on_board(&g, r, c));
    }
  }

  assert(g.snapped_count == 16);
  assert(g.is_solved);
  assert(g.puzzles_solved == initial_solved + 1);

  printf("[PASS] test_win_condition\n");
}

static void test_undo(void) {
  GameState g;
  game_init(&g, THEME_SUNSET);

  assert(game_pick_from_tray(&g, 2));
  assert(game_place_on_board(&g, 0, 2));
  assert(g.board[0][2] == 2);
  assert(g.snapped_count == 1);
  assert(g.moves_count == 1);

  /* Undo placement */
  assert(game_undo(&g));
  assert(g.board[0][2] == -1);
  assert(g.holding_piece == 2);
  assert(g.snapped_count == 0);
  assert(g.moves_count == 0);

  /* Undo pick from tray */
  assert(game_undo(&g));
  assert(g.holding_piece == -1);
  assert(g.tray_present[2]);

  printf("[PASS] test_undo\n");
}

static void test_themes_and_art(void) {
  for (int t = 0; t < THEME_COUNT; t++) {
    const JigsawTheme *theme = jigsaw_get_theme(t);
    assert(theme != NULL);
    assert(theme->title != NULL && strlen(theme->title) > 0);
    assert(theme->category != NULL && strlen(theme->category) > 0);

    /* Verify canvas cells are populated */
    for (int r = 0; r < ART_HEIGHT; r++) {
      for (int c = 0; c < ART_WIDTH; c++) {
        assert(strlen(theme->canvas[r][c].glyph) > 0);
      }
    }
  }

  printf("[PASS] test_themes_and_art\n");
}

static void test_render_and_mouse_hit(void) {
  GameState g;
  game_init(&g, THEME_SUNSET);

  int r = -1, c = -1;
  /* Top-left slot A1 is at x=7..14, y=5..7 */
  assert(render_get_board_slot_at_pos(8, 6, &r, &c));
  assert(r == 0 && c == 0);

  /* Bottom-right slot D4 is at x=31..38, y=14..16 */
  assert(render_get_board_slot_at_pos(35, 15, &r, &c));
  assert(r == 3 && c == 3);

  /* Outside board */
  assert(!render_get_board_slot_at_pos(2, 2, &r, &c));

  /* Tray hit testing */
  int t_idx = -1;
  assert(render_get_tray_slot_at_pos(48, 11, &t_idx));
  assert(t_idx == 0);

  /* Canary check for render_game */
  volatile int canary = 0x5A5A5A5A;
  render_game(&g);
  assert(canary == 0x5A5A5A5A);

  printf("[PASS] test_render_and_mouse_hit\n");
}

int main(void) {
  printf("Running Jigsaw Snap CLI unit tests...\n\n");

  test_init_and_board_setup();
  test_pick_and_place_snap();
  test_swap_pieces();
  test_return_to_tray();
  test_win_condition();
  test_undo();
  test_themes_and_art();
  test_render_and_mouse_hit();

  printf("\nAll Jigsaw Snap CLI unit tests PASSED successfully!\n");
  return 0;
}
