#include "config.h"
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

/* Helper to set a cell with fg, bg, and glyph */
static void set_cell(PixelCell *c, const char *fg, const char *bg, const char *glyph) {
  snprintf(c->fg_color, sizeof(c->fg_color), "%s", fg);
  snprintf(c->bg_color, sizeof(c->bg_color), "%s", bg);
  snprintf(c->glyph, sizeof(c->glyph), "%s", glyph);
}

/* Helper to fill a rectangle in a canvas */
static void fill_rect(PixelCell canvas[ART_HEIGHT][ART_WIDTH], int r1, int c1, int r2, int c2,
                      const char *fg, const char *bg, const char *glyph) {
  for (int r = r1; r <= r2 && r < ART_HEIGHT; r++) {
    for (int c = c1; c <= c2 && c < ART_WIDTH; c++) {
      set_cell(&canvas[r][c], fg, bg, glyph);
    }
  }
}

/* Helper to write text into a canvas */
static void write_text(PixelCell canvas[ART_HEIGHT][ART_WIDTH], int r, int start_c,
                       const char *text, const char *fg, const char *bg) {
  if (r < 0 || r >= ART_HEIGHT) return;
  int c = start_c;
  for (int i = 0; text[i] != '\0' && c < ART_WIDTH; i++) {
    char g[2] = { text[i], '\0' };
    set_cell(&canvas[r][c], fg, bg, g);
    c++;
  }
}

JigsawTheme JIGSAW_THEMES[THEME_COUNT];
static bool s_themes_initialized = false;

static void build_sunset_theme(JigsawTheme *t) {
  t->title = "Sunset Peak";
  t->category = "Scenic Nature";

  /* Rows 0..2: Deep twilight violet-navy with stars */
  fill_rect(t->canvas, 0, 0, 2, ART_WIDTH - 1, "\033[38;2;140;130;200m", "\033[48;2;25;20;50m", " ");
  set_cell(&t->canvas[0][4], "\033[1;38;2;255;255;200m", "\033[48;2;25;20;50m", "✦");
  set_cell(&t->canvas[1][12], "\033[1;38;2;255;255;255m", "\033[48;2;25;20;50m", "·");
  set_cell(&t->canvas[0][20], "\033[1;38;2;255;240;180m", "\033[48;2;25;20;50m", "✦");
  set_cell(&t->canvas[1][28], "\033[1;38;2;255;255;255m", "\033[48;2;25;20;50m", "·");

  /* Rows 3..5: Crimson to warm golden sunset glow */
  fill_rect(t->canvas, 3, 0, 3, ART_WIDTH - 1, "\033[38;2;230;70;90m", "\033[48;2;90;25;60m", "░");
  fill_rect(t->canvas, 4, 0, 4, ART_WIDTH - 1, "\033[38;2;255;130;60m", "\033[48;2;160;50;40m", "▒");
  fill_rect(t->canvas, 5, 0, 5, ART_WIDTH - 1, "\033[38;2;255;210;70m", "\033[48;2;210;90;30m", " ");

  /* Glowing evening sun at (4, 14..17) */
  fill_rect(t->canvas, 4, 13, 5, 18, "\033[1;38;2;255;255;220m", "\033[48;2;255;200;60m", "█");

  /* Rows 6..9: Snow-capped majestic mountain silhouette */
  /* Sky background for mountain rows */
  fill_rect(t->canvas, 6, 0, 8, ART_WIDTH - 1, "\033[38;2;255;160;70m", "\033[48;2;180;70;40m", " ");
  /* Mountain slopes */
  fill_rect(t->canvas, 6, 12, 6, 19, "\033[1;38;2;255;255;255m", "\033[48;2;220;225;240m", "▲");
  fill_rect(t->canvas, 7, 9, 7, 22, "\033[1;38;2;240;245;255m", "\033[48;2;120;130;165m", "█");
  fill_rect(t->canvas, 8, 6, 8, 25, "\033[38;2;80;90;130m", "\033[48;2;45;55;85m", "█");
  fill_rect(t->canvas, 9, 3, 9, 28, "\033[38;2;50;60;95m", "\033[48;2;30;40;65m", "█");

  /* Left & Right secondary peaks */
  fill_rect(t->canvas, 8, 0, 9, 5, "\033[38;2;60;70;105m", "\033[48;2;35;45;75m", "█");
  fill_rect(t->canvas, 8, 26, 9, 31, "\033[38;2;60;70;105m", "\033[48;2;35;45;75m", "█");

  /* Rows 10..11: Pine forest edge & lake reflection */
  fill_rect(t->canvas, 10, 0, 10, ART_WIDTH - 1, "\033[1;38;2;30;140;70m", "\033[48;2;15;50;25m", "▲");
  fill_rect(t->canvas, 11, 0, 11, ART_WIDTH - 1, "\033[1;38;2;70;180;220m", "\033[48;2;20;60;90m", "≈");
}

static void build_castle_theme(JigsawTheme *t) {
  t->title = "Cyber Citadel";
  t->category = "Synthwave Pixel Art";

  /* Sky: Deep indigo to hot magenta */
  fill_rect(t->canvas, 0, 0, 4, ART_WIDTH - 1, "\033[38;2;0;220;255m", "\033[48;2;20;10;45m", " ");
  /* Big neon moon at rows 1..3, cols 13..18 */
  fill_rect(t->canvas, 1, 13, 3, 18, "\033[1;38;2;255;255;180m", "\033[48;2;255;110;190m", "█");

  /* Citadel towers: rows 4..9 */
  fill_rect(t->canvas, 4, 0, 9, ART_WIDTH - 1, "\033[38;2;180;40;140m", "\033[48;2;35;15;55m", " ");
  /* Main tower: cols 10..21 */
  fill_rect(t->canvas, 4, 10, 9, 21, "\033[1;38;2;0;240;255m", "\033[48;2;60;20;80m", "█");
  /* Castle battlements / crenellations */
  for (int c = 10; c <= 21; c += 2) {
    set_cell(&t->canvas[4][c], "\033[1;38;2;255;80;180m", "\033[48;2;60;20;80m", "▀");
  }
  /* Flanking towers */
  fill_rect(t->canvas, 5, 2, 9, 7, "\033[1;38;2;255;70;180m", "\033[48;2;45;15;65m", "█");
  fill_rect(t->canvas, 5, 24, 9, 29, "\033[1;38;2;255;70;180m", "\033[48;2;45;15;65m", "█");

  /* Glowing cyan windows */
  fill_rect(t->canvas, 6, 12, 7, 13, "\033[1;38;2;255;255;255m", "\033[48;2;0;230;255m", "░");
  fill_rect(t->canvas, 6, 18, 7, 19, "\033[1;38;2;255;255;255m", "\033[48;2;0;230;255m", "░");
  fill_rect(t->canvas, 8, 14, 9, 17, "\033[1;38;2;255;255;255m", "\033[48;2;255;220;50m", "█"); /* Golden gate */

  /* Digital neon grid floor: rows 10..11 */
  fill_rect(t->canvas, 10, 0, 10, ART_WIDTH - 1, "\033[1;38;2;0;255;240m", "\033[48;2;20;5;35m", "═");
  fill_rect(t->canvas, 11, 0, 11, ART_WIDTH - 1, "\033[1;38;2;255;40;180m", "\033[48;2;10;0;25m", "┼");
}

static void build_space_theme(JigsawTheme *t) {
  t->title = "Space Voyager";
  t->category = "Cosmic Retro";

  /* Cosmic deep space background */
  fill_rect(t->canvas, 0, 0, ART_HEIGHT - 1, ART_WIDTH - 1, "\033[38;2;180;190;255m", "\033[48;2;10;10;25m", " ");
  /* Stars */
  set_cell(&t->canvas[1][2], "\033[1;38;2;255;255;255m", "\033[48;2;10;10;25m", "✦");
  set_cell(&t->canvas[3][8], "\033[1;38;2;140;220;255m", "\033[48;2;10;10;25m", "·");
  set_cell(&t->canvas[0][28], "\033[1;38;2;255;255;180m", "\033[48;2;10;10;25m", "✦");
  set_cell(&t->canvas[8][3], "\033[1;38;2;255;255;255m", "\033[48;2;10;10;25m", "·");
  set_cell(&t->canvas[10][29], "\033[1;38;2;255;220;200m", "\033[48;2;10;10;25m", "✦");

  /* Emerald Saturn ringed planet at top right: rows 1..3, cols 21..27 */
  fill_rect(t->canvas, 1, 23, 3, 26, "\033[1;38;2;120;255;160m", "\033[48;2;30;120;60m", "█");
  fill_rect(t->canvas, 2, 20, 2, 29, "\033[1;38;2;255;240;120m", "\033[48;2;10;10;25m", "═");

  /* Rocket Ship flying upwards: rows 2..8, cols 11..19 */
  /* Nose cone */
  fill_rect(t->canvas, 2, 14, 2, 16, "\033[1;38;2;255;60;60m", "\033[48;2;200;30;30m", "▲");
  /* Fuselage */
  fill_rect(t->canvas, 3, 13, 6, 17, "\033[1;38;2;255;255;255m", "\033[48;2;210;220;235m", "█");
  /* Cockpit window */
  fill_rect(t->canvas, 4, 14, 4, 16, "\033[1;38;2;0;230;255m", "\033[48;2;0;140;210m", "●");
  /* Booster fins */
  fill_rect(t->canvas, 6, 11, 7, 12, "\033[1;38;2;255;60;60m", "\033[48;2;180;20;20m", "◄");
  fill_rect(t->canvas, 6, 18, 7, 19, "\033[1;38;2;255;60;60m", "\033[48;2;180;20;20m", "►");

  /* Fire exhaust plume: rows 7..11 */
  fill_rect(t->canvas, 7, 14, 8, 16, "\033[1;38;2;255;255;100m", "\033[48;2;255;180;20m", "█");
  fill_rect(t->canvas, 9, 13, 10, 17, "\033[1;38;2;255;120;30m", "\033[48;2;220;60;20m", "▒");
  fill_rect(t->canvas, 11, 14, 11, 16, "\033[1;38;2;255;50;40m", "\033[48;2;150;20;20m", "░");
}

static void build_cat_theme(JigsawTheme *t) {
  t->title = "Cozy Tabby";
  t->category = "Pixel Pet";

  /* Bedroom / Wallpaper background: warm lavender */
  fill_rect(t->canvas, 0, 0, 7, ART_WIDTH - 1, "\033[38;2;180;170;210m", "\033[48;2;45;35;60m", " ");
  /* Floating dream zZz */
  write_text(t->canvas, 1, 23, "z", "\033[1;38;2;255;255;200m", "\033[48;2;45;35;60m");
  write_text(t->canvas, 0, 25, "Z", "\033[1;38;2;255;240;150m", "\033[48;2;45;35;60m");

  /* Teal velvet cushion: rows 8..11, cols 3..28 */
  fill_rect(t->canvas, 8, 3, 11, 28, "\033[1;38;2;40;210;190m", "\033[48;2;15;75;80m", "█");
  /* Cushion gold fringe border */
  for (int c = 3; c <= 28; c += 2) {
    set_cell(&t->canvas[11][c], "\033[1;38;2;255;215;70m", "\033[48;2;15;75;80m", "♦");
  }

  /* Orange Tabby Cat sleeping: rows 4..8, cols 8..23 */
  /* Cat ears */
  set_cell(&t->canvas[4][10], "\033[1;38;2;255;140;40m", "\033[48;2;45;35;60m", "▲");
  set_cell(&t->canvas[4][13], "\033[1;38;2;255;140;40m", "\033[48;2;45;35;60m", "▲");
  /* Cat head & body */
  fill_rect(t->canvas, 5, 9, 8, 22, "\033[1;38;2;255;170;60m", "\033[48;2;215;105;25m", "█");
  /* Sleeping eyes & cute nose */
  write_text(t->canvas, 6, 10, "-.-", "\033[1;38;2;60;25;10m", "\033[48;2;215;105;25m");
  /* White chest patch */
  fill_rect(t->canvas, 7, 13, 8, 16, "\033[1;38;2;255;250;240m", "\033[48;2;240;230;220m", "█");
  /* Curled tail on the right */
  set_cell(&t->canvas[6][23], "\033[1;38;2;255;140;40m", "\033[48;2;45;35;60m", "╭");
  set_cell(&t->canvas[7][23], "\033[1;38;2;255;140;40m", "\033[48;2;15;75;80m", "╯");
}

static void build_numbers_theme(JigsawTheme *t) {
  t->title = "Chroma Matrix";
  t->category = "Number Challenge";

  /* 16 distinct vibrant gradient palette blocks */
  static const struct {
    const char *fg;
    const char *bg;
  } palette[16] = {
    { "\033[1;38;2;255;255;255m", "\033[48;2;180;30;40m"   }, /* 01: Crimson */
    { "\033[1;38;2;255;255;255m", "\033[48;2;210;60;30m"   }, /* 02: Rust Red */
    { "\033[1;38;2;255;255;255m", "\033[48;2;225;110;25m"  }, /* 03: Orange */
    { "\033[1;38;2;255;255;255m", "\033[48;2;215;160;20m"  }, /* 04: Amber */
    { "\033[1;38;2;255;255;255m", "\033[48;2;160;185;25m"  }, /* 05: Lime Gold */
    { "\033[1;38;2;255;255;255m", "\033[48;2;70;175;50m"   }, /* 06: Leaf Green */
    { "\033[1;38;2;255;255;255m", "\033[48;2;25;165;95m"   }, /* 07: Emerald */
    { "\033[1;38;2;255;255;255m", "\033[48;2;20;160;150m"  }, /* 08: Cyan Sea */
    { "\033[1;38;2;255;255;255m", "\033[48;2;25;135;190m"  }, /* 09: Azure */
    { "\033[1;38;2;255;255;255m", "\033[48;2;40;105;205m"  }, /* 10: Cobalt */
    { "\033[1;38;2;255;255;255m", "\033[48;2;85;80;210m"   }, /* 11: Indigo */
    { "\033[1;38;2;255;255;255m", "\033[48;2;135;60;200m"  }, /* 12: Purple */
    { "\033[1;38;2;255;255;255m", "\033[48;2;180;50;185m"  }, /* 13: Magenta */
    { "\033[1;38;2;255;255;255m", "\033[48;2;210;45;150m"  }, /* 14: Hot Pink */
    { "\033[1;38;2;255;255;255m", "\033[48;2;190;40;100m"  }, /* 15: Rose */
    { "\033[1;38;2;255;255;255m", "\033[48;2;140;40;70m"   }  /* 16: Wine */
  };

  for (int slot = 0; slot < 16; slot++) {
    int tr = slot / 4;
    int tc = slot % 4;
    int r1 = tr * TILE_HEIGHT;
    int c1 = tc * TILE_WIDTH;
    const char *fg = palette[slot].fg;
    const char *bg = palette[slot].bg;

    fill_rect(t->canvas, r1, c1, r1 + TILE_HEIGHT - 1, c1 + TILE_WIDTH - 1, fg, bg, " ");

    /* Top decorative tabs */
    char label[16];
    char coord[8];
    snprintf(coord, sizeof(coord), "%c%d", 'A' + tc, tr + 1);
    snprintf(label, sizeof(label), "#%02d", slot + 1);

    write_text(t->canvas, r1, c1 + 1, coord, "\033[1;38;2;255;255;180m", bg);
    write_text(t->canvas, r1 + 1, c1 + 2, label, fg, bg);
    write_text(t->canvas, r1 + 2, c1 + 2, "────", "\033[38;2;255;255;255;50m", bg);
  }
}

static void ensure_themes_init(void) {
  if (s_themes_initialized) return;
  for (int i = 0; i < THEME_COUNT; i++) {
    fill_rect(JIGSAW_THEMES[i].canvas, 0, 0, ART_HEIGHT - 1, ART_WIDTH - 1,
              "\033[38;2;180;180;180m", "\033[48;2;20;20;30m", " ");
  }
  build_sunset_theme(&JIGSAW_THEMES[0]);
  build_castle_theme(&JIGSAW_THEMES[1]);
  build_space_theme(&JIGSAW_THEMES[2]);
  build_cat_theme(&JIGSAW_THEMES[3]);
  build_numbers_theme(&JIGSAW_THEMES[4]);
  s_themes_initialized = true;
}

const JigsawTheme *jigsaw_get_theme(int theme_idx) {
  ensure_themes_init();
  if (theme_idx < 0 || theme_idx >= THEME_COUNT) theme_idx = 0;
  return &JIGSAW_THEMES[theme_idx];
}
