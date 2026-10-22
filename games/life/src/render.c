#include "render.h"
#include <stdio.h>
#include <string.h>

#define COLOR_RESET       "\033[0m"
#define COLOR_BORDER      "\033[38;2;90;145;200m"
#define COLOR_GRID        "\033[38;2;110;125;145m"
#define COLOR_TITLE       "\033[1;38;2;255;255;255m"
#define COLOR_GOLD        "\033[1;38;2;255;215;70m"
#define COLOR_CYAN        "\033[1;38;2;0;230;225m"
#define COLOR_DIM         "\033[38;2;140;155;175m"
#define COLOR_ALERT       "\033[1;38;2;255;85;85m"
#define COLOR_WIN         "\033[1;38;2;80;255;130m"

#define BG_DEAD           "\033[48;2;18;22;32m"
#define FG_DEAD           "\033[38;2;35;42;58m"

/* Age heat colors */
#define FG_NEWBORN        "\033[1;38;2;0;240;255m"
#define BG_NEWBORN        "\033[48;2;10;50;70m"

#define FG_YOUNG          "\033[1;38;2;80;255;120m"
#define BG_YOUNG          "\033[48;2;15;55;25m"

#define FG_MATURE         "\033[1;38;2;255;210;50m"
#define BG_MATURE         "\033[48;2;60;50;15m"

#define FG_ANCIENT        "\033[1;38;2;255;70;180m"
#define BG_ANCIENT        "\033[48;2;60;15;45m"

#define BG_CURSOR         "\033[48;2;40;130;180m"
#define FG_CURSOR         "\033[1;38;2;255;255;255m"

#define TABLE_BORDER_HORIZ "════════════════════════════════════════════════════════════════════════════════════════════"

void render_init(void) {
  printf("\033[2J\033[H");
  fflush(stdout);
}

void render_cleanup(void) {
  printf("%s", COLOR_RESET);
  fflush(stdout);
}

static int visible_length(const char *s) {
  if (!s) return 0;
  int len = 0;
  bool in_escape = false;
  for (int i = 0; s[i] != '\0'; i++) {
    if (s[i] == '\033') {
      in_escape = true;
    } else if (in_escape) {
      if ((s[i] >= 'A' && s[i] <= 'Z') || (s[i] >= 'a' && s[i] <= 'z')) {
        in_escape = false;
      }
    } else {
      unsigned char c = (unsigned char)s[i];
      if ((c & 0xC0) != 0x80) {
        len++;
      }
    }
  }
  return len;
}

static void print_table_line(const char *content) {
  int vis = visible_length(content);
  int pad = INNER_WIDTH - vis;
  if (pad < 0) pad = 0;
  printf("  %s║%s%s%*s%s║%s\033[K\n",
         COLOR_BORDER, COLOR_RESET, content, pad, "", COLOR_BORDER, COLOR_RESET);
}

static void print_split_line(const char *left_col, const char *right_col) {
  int vis_l = visible_length(left_col);
  int pad_l = LEFT_PANEL_WIDTH - vis_l;
  if (pad_l < 0) pad_l = 0;

  int vis_r = visible_length(right_col);
  int pad_r = RIGHT_PANEL_WIDTH - vis_r;
  if (pad_r < 0) pad_r = 0;

  printf("  %s║%s%s%*s%s│%s%s%*s%s║%s\033[K\n",
         COLOR_BORDER, COLOR_RESET,
         left_col, pad_l, "",
         COLOR_BORDER, COLOR_RESET,
         right_col, pad_r, "",
         COLOR_BORDER, COLOR_RESET);
}

bool render_get_cell_at_pos(int mouse_x, int mouse_y, int *out_r, int *out_c) {
  /* Arena cell rows: mouse_y in [4..21] (18 rows) */
  /* Arena cell cols: mouse_x in [4..63] (30 cols * 2 chars) */
  if (mouse_y < 4 || mouse_y > 21 || mouse_x < 4 || mouse_x > 63) {
    return false;
  }
  int r = mouse_y - 4;
  int c = (mouse_x - 4) / CELL_CHAR_WIDTH;
  if (r >= 0 && r < GRID_ROWS && c >= 0 && c < GRID_COLS) {
    if (out_r) *out_r = r;
    if (out_c) *out_c = c;
    return true;
  }
  return false;
}

static void format_cell(const GameState *g, int r, int c, char *out, size_t size) {
  unsigned char age = g->grid[r][c];
  bool is_cur = (g->cursor_r == r && g->cursor_c == c);

  if (is_cur) {
    if (age > 0) {
      snprintf(out, size, "%s%s[]%s", BG_CURSOR, FG_CURSOR, COLOR_RESET);
    } else {
      snprintf(out, size, "%s%s··%s", BG_CURSOR, FG_CURSOR, COLOR_RESET);
    }
    return;
  }

  if (age == 0) {
    snprintf(out, size, "%s%s  %s", BG_DEAD, FG_DEAD, COLOR_RESET);
  } else if (age == 1) {
    snprintf(out, size, "%s%s██%s", BG_NEWBORN, FG_NEWBORN, COLOR_RESET);
  } else if (age <= 4) {
    snprintf(out, size, "%s%s██%s", BG_YOUNG, FG_YOUNG, COLOR_RESET);
  } else if (age <= 14) {
    snprintf(out, size, "%s%s██%s", BG_MATURE, FG_MATURE, COLOR_RESET);
  } else {
    snprintf(out, size, "%s%s██%s", BG_ANCIENT, FG_ANCIENT, COLOR_RESET);
  }
}

static void format_right_panel_row(const GameState *g, int row_idx, char *out, size_t size) {
  out[0] = '\0';
  const LifePreset *preset = &LIFE_PRESETS[g->selected_preset];

  switch (row_idx) {
    case 0:
      snprintf(out, size, " %sSIMULATION DASHBOARD%s", COLOR_GOLD, COLOR_RESET);
      break;

    case 1:
      if (g->is_running) {
        snprintf(out, size, " Status: %s▶ RUNNING%s", COLOR_WIN, COLOR_RESET);
      } else {
        snprintf(out, size, " Status: %s⏸ PAUSED%s", COLOR_ALERT, COLOR_RESET);
      }
      break;

    case 2:
      snprintf(out, size, " Gen:    %s%-18llu%s", COLOR_CYAN, g->generation, COLOR_RESET);
      break;

    case 3: {
      int pct = (g->population * 100) / TOTAL_CELLS;
      snprintf(out, size, " Pop:    %s%3d%s (%2d%%)  Peak:%s%3d%s",
               COLOR_TITLE, g->population, COLOR_RESET, pct,
               COLOR_GOLD, g->peak_population, COLOR_RESET);
      break;
    }

    case 4: {
      /* Population mini-bar */
      char bar[19];
      int filled = (g->population * 18) / TOTAL_CELLS;
      for (int i = 0; i < 18; i++) {
        bar[i] = (i < filled) ? '=' : '-';
      }
      bar[18] = '\0';
      snprintf(out, size, " [%s%s%s]", COLOR_CYAN, bar, COLOR_RESET);
      break;
    }

    case 5:
      snprintf(out, size, " Wrap:   %s%-18s%s",
               COLOR_GOLD,
               (g->wrap_mode == WRAP_TOROIDAL) ? "TOROIDAL (Wrap)" : "BOUNDED (Wall)",
               COLOR_RESET);
      break;

    case 6: {
      static const int tps_map[10] = { 1, 2, 4, 6, 10, 14, 20, 28, 40, 60 };
      snprintf(out, size, " Speed:  %sLv %-2d (%2d tps)%s",
               COLOR_TITLE, g->speed_level, tps_map[g->speed_level - 1], COLOR_RESET);
      break;
    }

    case 7: {
      /* Speed gauge */
      char gauge[11];
      for (int i = 0; i < 10; i++) {
        gauge[i] = (i < g->speed_level) ? '|' : '.';
      }
      gauge[10] = '\0';
      snprintf(out, size, " [%s%s%s] [+/-] Speed", COLOR_GOLD, gauge, COLOR_RESET);
      break;
    }

    case 8:
      snprintf(out, size, " %s───────────────────────────%s", COLOR_GRID, COLOR_RESET);
      break;

    case 9:
      snprintf(out, size, " %sPRESET STAMP [P]:%s", COLOR_GOLD, COLOR_RESET);
      break;

    case 10:
      snprintf(out, size, " %s► %-23s%s", COLOR_CYAN, preset->name, COLOR_RESET);
      break;

    case 11:
      snprintf(out, size, " %s(%-25s)%s", COLOR_DIM, preset->category, COLOR_RESET);
      break;

    case 12:
    case 13:
    case 14:
    case 15: {
      /* Mini pattern preview (4 rows of 5 cells) */
      int pr = row_idx - 12;
      int pos = snprintf(out, size, "   ");
      if (pr < preset->height) {
        int max_cols = (preset->width > 12) ? 12 : preset->width;
        for (int pc = 0; pc < max_cols; pc++) {
          char ch = preset->pattern[pr][pc];
          if (ch == 'O' || ch == '#' || ch == '1') {
            pos += snprintf(out + pos, size - pos, "%s██%s", COLOR_WIN, COLOR_RESET);
          } else {
            pos += snprintf(out + pos, size - pos, "%s··%s", COLOR_DIM, COLOR_RESET);
          }
        }
      }
      break;
    }

    case 16:
      snprintf(out, size, " %s───────────────────────────%s", COLOR_GRID, COLOR_RESET);
      break;

    case 17:
      snprintf(out, size, " %s[Space]%s Run/Pause %s[.]%s Step",
               COLOR_GOLD, COLOR_RESET, COLOR_CYAN, COLOR_RESET);
      break;

    case 18:
      snprintf(out, size, " %s[Click/Enter]%s Draw %s[U]%s Undo",
               COLOR_CYAN, COLOR_RESET, COLOR_GOLD, COLOR_RESET);
      break;

    case 19:
      snprintf(out, size, " %s[X/B]%s Stamp  %s[C]%s Clr %s[R]%s Seed",
               COLOR_GOLD, COLOR_RESET, COLOR_CYAN, COLOR_RESET, COLOR_GOLD, COLOR_RESET);
      break;

    default:
      break;
  }
}

void render_game(const GameState *g) {
  printf("\033[H");

  /* Line 1: Top outer border */
  printf("  %s╔" TABLE_BORDER_HORIZ "╗%s\033[K\n", COLOR_BORDER, COLOR_RESET);

  /* Line 2: Header bar */
  char header[512];
  snprintf(header, sizeof(header),
           " %s🧬 CONWAY'S GAME OF LIFE%s │ %sGEN: %-10llu%s │ %sPOP: %-4d%s │ %sSTATUS: %s%s",
           COLOR_TITLE, COLOR_RESET,
           COLOR_CYAN, g->generation, COLOR_RESET,
           COLOR_GOLD, g->population, COLOR_RESET,
           g->is_running ? COLOR_WIN : COLOR_ALERT,
           g->is_running ? "RUNNING" : "PAUSED",
           COLOR_RESET);
  print_table_line(header);

  /* Line 3: Top separator */
  printf("  %s╠" TABLE_BORDER_HORIZ "╣%s\033[K\n", COLOR_BORDER, COLOR_RESET);

  /* Lines 4..23 (20 rows): Arena (Left 62 cols) + Divider + Dashboard (Right 29 cols) */
  for (int row = 0; row < 20; row++) {
    char left_buf[4096];
    char right_buf[512];

    if (row == 0) {
      /* Top arena frame */
      snprintf(left_buf, sizeof(left_buf), " %s┌────────────────────────────────────────────────────────────┐%s ",
               COLOR_GRID, COLOR_RESET);
    } else if (row >= 1 && row <= 18) {
      int grid_r = row - 1;
      int pos = snprintf(left_buf, sizeof(left_buf), " %s│%s", COLOR_GRID, COLOR_RESET);

      for (int grid_c = 0; grid_c < GRID_COLS; grid_c++) {
        char cell_str[128];
        format_cell(g, grid_r, grid_c, cell_str, sizeof(cell_str));
        pos += snprintf(left_buf + pos, sizeof(left_buf) - pos, "%s", cell_str);
      }

      snprintf(left_buf + pos, sizeof(left_buf) - pos, "%s│%s ", COLOR_GRID, COLOR_RESET);
    } else {
      /* Bottom arena frame */
      snprintf(left_buf, sizeof(left_buf), " %s└────────────────────────────────────────────────────────────┘%s ",
               COLOR_GRID, COLOR_RESET);
    }

    format_right_panel_row(g, row, right_buf, sizeof(right_buf));
    print_split_line(left_buf, right_buf);
  }

  /* Line 24: Bottom separator */
  printf("  %s╠" TABLE_BORDER_HORIZ "╣%s\033[K\n", COLOR_BORDER, COLOR_RESET);

  /* Line 25: Status & Controls footer bar */
  char footer[512];
  char msg_trunc[32];
  snprintf(msg_trunc, sizeof(msg_trunc), "%.28s", g->status_msg);
  snprintf(footer, sizeof(footer),
           " %s[Space]%sRun %s[.]%sStep %s[U]%sUndo %s[P]%sPreset %s[X]%sStamp %s[Q]%sQuit │ %s%-28s%s",
           COLOR_GOLD, COLOR_RESET,
           COLOR_CYAN, COLOR_RESET,
           COLOR_GOLD, COLOR_RESET,
           COLOR_CYAN, COLOR_RESET,
           COLOR_GOLD, COLOR_RESET,
           COLOR_CYAN, COLOR_RESET,
           COLOR_TITLE, msg_trunc, COLOR_RESET);
  print_table_line(footer);

  /* Line 26: Bottom outer border */
  printf("  %s╚" TABLE_BORDER_HORIZ "╝%s\033[K\n", COLOR_BORDER, COLOR_RESET);

  fflush(stdout);
}
