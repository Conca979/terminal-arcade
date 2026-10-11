#include "render.h"
#include <stdio.h>
#include <string.h>

#define COLOR_RESET "\033[0m"
#define COLOR_ARENA_BORDER "\033[1;38;2;70;130;210m"
#define COLOR_SIDE_BORDER "\033[38;2;80;110;160m"
#define COLOR_TITLE "\033[1;38;2;255;255;255m"
#define COLOR_LABEL "\033[1;38;2;160;185;220m"
#define COLOR_VALUE "\033[1;38;2;255;220;50m"
#define COLOR_ACCENT "\033[1;38;2;0;230;240m"
#define COLOR_DANGER "\033[1;38;2;255;65;65m"
#define COLOR_ORB "\033[1;38;2;255;225;60m"
#define COLOR_BOMB "\033[1;38;2;255;90;40m"
#define COLOR_LASER "\033[1;38;2;190;70;255m"
#define COLOR_BALL "\033[1;38;2;0;245;255m"
#define COLOR_GUIDE "\033[1;38;2;245;235;120m"

void render_init(void) {
  printf("\033[2J\033[H");
  fflush(stdout);
}

void render_cleanup(void) {
  printf(COLOR_RESET);
  fflush(stdout);
}

static const char *get_brick_color(int hp, int max_hp) {
  float ratio = (max_hp > 0) ? ((float)hp / (float)max_hp) : 1.0f;
  if (ratio > 0.70f) {
    return "\033[1;38;2;255;70;70m";   // Red / High
  } else if (ratio > 0.35f) {
    return "\033[1;38;2;255;180;40m";  // Amber / Medium
  } else {
    return "\033[1;38;2;75;230;120m";  // Lime / Low
  }
}

void render_frame(const GameState *game, unsigned long long current_time) {
  (void)current_time;
  static char buffer[65536];
  int offset = 0;

  offset += snprintf(buffer + offset, sizeof(buffer) - offset, "\033[H");

  // Title Banner - exactly 91 columns wide (Arena 50 + Gap 3 + Sidebar 38)
  offset += snprintf(
    buffer + offset, sizeof(buffer) - offset,
    "  %s╔═════════════════════════════════════════════════════════════════════════════════════════╗%s\033[K\n"
    "  %s║%s                      %sN E O N   B A L L S   B R I C K S   B R E A K E R%s                      %s║%s\033[K\n"
    "  %s╚═════════════════════════════════════════════════════════════════════════════════════════╝%s\033[K\n",
    COLOR_ARENA_BORDER, COLOR_RESET,
    COLOR_ARENA_BORDER, COLOR_RESET, COLOR_TITLE, COLOR_RESET, COLOR_ARENA_BORDER, COLOR_RESET,
    COLOR_ARENA_BORDER, COLOR_RESET
  );

  // Status Bar
  const char *phase_str = "AIMING";
  if (game->phase == PHASE_FIRING) phase_str = "FIRING";
  else if (game->phase == PHASE_GAME_OVER) phase_str = "GAME OVER";

  const char *speed_str = game->turbo_mode ? "\033[1;31m[3x TURBO]\033[0m" : "\033[1;32m[1x NORMAL]\033[0m";

  offset += snprintf(
    buffer + offset, sizeof(buffer) - offset,
    "  %sROUND:%s %s%03d%s   "
    "%sBALLS:%s %s%-3d%s   "
    "%sSCORE:%s %s%06d%s   "
    "%sBEST:%s %s%06d%s   "
    "%sSTATE:%s \033[1;36m[%-9s]\033[0m   "
    "%sSPEED:%s %s\033[K\n\033[K\n",
    COLOR_LABEL, COLOR_RESET, COLOR_VALUE, game->round, COLOR_RESET,
    COLOR_LABEL, COLOR_RESET, COLOR_VALUE, game->total_balls, COLOR_RESET,
    COLOR_LABEL, COLOR_RESET, COLOR_VALUE, game->score, COLOR_RESET,
    COLOR_LABEL, COLOR_RESET, COLOR_VALUE, game->high_score, COLOR_RESET,
    COLOR_LABEL, COLOR_RESET, phase_str,
    COLOR_LABEL, COLOR_RESET, speed_str
  );

  // Prepare 36 Sidebar lines (each exactly 34 columns wide inside)
  char side[36][256];

  snprintf(side[0], sizeof(side[0]), "%s┌── RUN TELEMETRY ─────────────────┐%s", COLOR_SIDE_BORDER, COLOR_RESET);
  snprintf(side[1], sizeof(side[1]), "%s│%s  Round:         \033[1;33m%-18d\033[0m%s│%s", COLOR_SIDE_BORDER, COLOR_RESET, game->round, COLOR_SIDE_BORDER, COLOR_RESET);
  snprintf(side[2], sizeof(side[2]), "%s│%s  Swarm Balls:   \033[1;36m%-18d\033[0m%s│%s", COLOR_SIDE_BORDER, COLOR_RESET, game->total_balls, COLOR_SIDE_BORDER, COLOR_RESET);
  snprintf(side[3], sizeof(side[3]), "%s│%s  Active Flight: %-18d%s│%s", COLOR_SIDE_BORDER, COLOR_RESET, game->balls_active, COLOR_SIDE_BORDER, COLOR_RESET);
  snprintf(side[4], sizeof(side[4]), "%s│%s  Current Score: %-18d%s│%s", COLOR_SIDE_BORDER, COLOR_RESET, game->score, COLOR_SIDE_BORDER, COLOR_RESET);
  snprintf(side[5], sizeof(side[5]), "%s│%s  All-Time Best: \033[1;32m%-18d\033[0m%s│%s", COLOR_SIDE_BORDER, COLOR_RESET, game->high_score, COLOR_SIDE_BORDER, COLOR_RESET);

  snprintf(side[6], sizeof(side[6]), "%s├── SWARM UPGRADES ────────────────┤%s", COLOR_SIDE_BORDER, COLOR_RESET);
  snprintf(side[7], sizeof(side[7]), "%s│%s  Next Turn:     \033[1;33m+%-17d\033[0m%s│%s", COLOR_SIDE_BORDER, COLOR_RESET, game->collected_balls, COLOR_SIDE_BORDER, COLOR_RESET);
  snprintf(side[8], sizeof(side[8]), "%s│%s  Launcher X:    %-18.1f%s│%s", COLOR_SIDE_BORDER, COLOR_RESET, game->launcher_x, COLOR_SIDE_BORDER, COLOR_RESET);

  float deg = game->aim_angle * (180.0f / 3.14159265f);
  snprintf(side[9], sizeof(side[9]), "%s│%s  Aim Angle:     %-17.1f°%s│%s", COLOR_SIDE_BORDER, COLOR_RESET, -deg, COLOR_SIDE_BORDER, COLOR_RESET);
  snprintf(side[10], sizeof(side[10]), "%s│%s  Physics Speed: %-18s%s│%s", COLOR_SIDE_BORDER, COLOR_RESET, game->turbo_mode ? "3x TURBO" : "1x NORMAL", COLOR_SIDE_BORDER, COLOR_RESET);

  snprintf(side[11], sizeof(side[11]), "%s├── BRICK LEGEND ──────────────────┤%s", COLOR_SIDE_BORDER, COLOR_RESET);
  snprintf(side[12], sizeof(side[12]), "%s│%s  [ 24 ] Normal Numbered Brick     %s│%s", COLOR_SIDE_BORDER, COLOR_RESET, COLOR_SIDE_BORDER, COLOR_RESET);
  snprintf(side[13], sizeof(side[13]), "%s│%s  \033[1;33m  ⦿   \033[0m Extra Ball Orb (+1 Ball)  %s│%s", COLOR_SIDE_BORDER, COLOR_RESET, COLOR_SIDE_BORDER, COLOR_RESET);
  snprintf(side[14], sizeof(side[14]), "%s│%s  \033[1;31m  💣  \033[0m Area Bomb (3x3 Blast)     %s│%s", COLOR_SIDE_BORDER, COLOR_RESET, COLOR_SIDE_BORDER, COLOR_RESET);
  snprintf(side[15], sizeof(side[15]), "%s│%s  \033[1;36m  ══  \033[0m Row Laser Clearance       %s│%s", COLOR_SIDE_BORDER, COLOR_RESET, COLOR_SIDE_BORDER, COLOR_RESET);
  snprintf(side[16], sizeof(side[16]), "%s│%s  \033[1;35m  ║║  \033[0m Column Laser Clearance    %s│%s", COLOR_SIDE_BORDER, COLOR_RESET, COLOR_SIDE_BORDER, COLOR_RESET);

  snprintf(side[17], sizeof(side[17]), "%s├── RUN MANAGEMENT ────────────────┤%s", COLOR_SIDE_BORDER, COLOR_RESET);
  snprintf(side[18], sizeof(side[18]), "%s│%s  Mode:          ENDLESS ONLY      %s│%s", COLOR_SIDE_BORDER, COLOR_RESET, COLOR_SIDE_BORDER, COLOR_RESET);
  snprintf(side[19], sizeof(side[19]), "%s│%s  Save State:    AUTO-PERSISTENT   %s│%s", COLOR_SIDE_BORDER, COLOR_RESET, COLOR_SIDE_BORDER, COLOR_RESET);
  snprintf(side[20], sizeof(side[20]), "%s│%s  Run Status:    %-18s%s│%s", COLOR_SIDE_BORDER, COLOR_RESET, game->resumed_game ? "RESUMED RUN" : "ACTIVE RUN", COLOR_SIDE_BORDER, COLOR_RESET);
  snprintf(side[21], sizeof(side[21]), "%s│%s  Danger Line:   ROW 24            %s│%s", COLOR_SIDE_BORDER, COLOR_RESET, COLOR_SIDE_BORDER, COLOR_RESET);

  snprintf(side[22], sizeof(side[22]), "%s├── CONTROLS ──────────────────────┤%s", COLOR_SIDE_BORDER, COLOR_RESET);
  snprintf(side[23], sizeof(side[23]), "%s│%s  MOUSE:                           %s│%s", COLOR_SIDE_BORDER, COLOR_RESET, COLOR_SIDE_BORDER, COLOR_RESET);
  snprintf(side[24], sizeof(side[24]), "%s│%s    Move Mouse   : Aim Trajectory  %s│%s", COLOR_SIDE_BORDER, COLOR_RESET, COLOR_SIDE_BORDER, COLOR_RESET);
  snprintf(side[25], sizeof(side[25]), "%s│%s    Left Click   : Launch Swarm    %s│%s", COLOR_SIDE_BORDER, COLOR_RESET, COLOR_SIDE_BORDER, COLOR_RESET);
  snprintf(side[26], sizeof(side[26]), "%s│%s    Right Click  : 3x Turbo Speed  %s│%s", COLOR_SIDE_BORDER, COLOR_RESET, COLOR_SIDE_BORDER, COLOR_RESET);
  snprintf(side[27], sizeof(side[27]), "%s│%s  KEYBOARD:                        %s│%s", COLOR_SIDE_BORDER, COLOR_RESET, COLOR_SIDE_BORDER, COLOR_RESET);
  snprintf(side[28], sizeof(side[28]), "%s│%s    ← / → / WASD : Adjust Aim Angle %s│%s", COLOR_SIDE_BORDER, COLOR_RESET, COLOR_SIDE_BORDER, COLOR_RESET);
  snprintf(side[29], sizeof(side[29]), "%s│%s    Space / Enter: Fire Ball Swarm %s│%s", COLOR_SIDE_BORDER, COLOR_RESET, COLOR_SIDE_BORDER, COLOR_RESET);
  snprintf(side[30], sizeof(side[30]), "%s│%s    F / Space    : 3x Turbo Speed  %s│%s", COLOR_SIDE_BORDER, COLOR_RESET, COLOR_SIDE_BORDER, COLOR_RESET);
  snprintf(side[31], sizeof(side[31]), "%s│%s    R            : New Endless Run %s│%s", COLOR_SIDE_BORDER, COLOR_RESET, COLOR_SIDE_BORDER, COLOR_RESET);
  snprintf(side[32], sizeof(side[32]), "%s│%s    Q / Esc      : Save and Exit   %s│%s", COLOR_SIDE_BORDER, COLOR_RESET, COLOR_SIDE_BORDER, COLOR_RESET);

  snprintf(side[33], sizeof(side[33]), "%s├── TACTICAL TIP ──────────────────┤%s", COLOR_SIDE_BORDER, COLOR_RESET);
  snprintf(side[34], sizeof(side[34]), "%s│%s  Aim through tight gaps into the   %s│%s", COLOR_SIDE_BORDER, COLOR_RESET, COLOR_SIDE_BORDER, COLOR_RESET);
  snprintf(side[35], sizeof(side[35]), "%s└──────────────────────────────────┘%s", COLOR_SIDE_BORDER, COLOR_RESET);

  // Render Arena Line 0 (Top border) side-by-side with Sidebar Line 0
  int side_idx = 0;
  offset += snprintf(
    buffer + offset, sizeof(buffer) - offset,
    "  %s╔════════════════════════════════════════════════╗%s   %s\033[K\n",
    COLOR_ARENA_BORDER, COLOR_RESET, side[side_idx++]
  );

  // Render 34 interior rows of Arena (Rows 0 to 33)
  for (int r = 0; r < ARENA_HEIGHT; r++) {
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "  %s║%s", COLOR_ARENA_BORDER, COLOR_RESET);

    // Build row character buffer (48 characters)
    char row_chars[ARENA_WIDTH + 1];
    memset(row_chars, ' ', ARENA_WIDTH);
    row_chars[ARENA_WIDTH] = '\0';

    // 1. Danger line indicator
    if (r == DANGER_ROW) {
      for (int i = 0; i < ARENA_WIDTH; i += 2) {
        row_chars[i] = '-';
      }
    }

    // 2. Trajectory Laser Guide
    if (game->phase == PHASE_AIMING) {
      for (int i = 0; i < game->laser_guide.count; i++) {
        int lx = (int)game->laser_guide.points[i].x;
        int ly = (int)game->laser_guide.points[i].y;
        if (ly == r && lx >= 0 && lx < ARENA_WIDTH) {
          row_chars[lx] = '.';
        }
      }
    }

    // 3. Active balls in flight
    for (int i = 0; i < MAX_BALLS; i++) {
      if (game->balls[i].active) {
        int bx = (int)game->balls[i].x;
        int by = (int)game->balls[i].y;
        if (by == r && bx >= 0 && bx < ARENA_WIDTH) {
          row_chars[bx] = 'o';
        }
      }
    }

    // 4. Launcher icon at bottom row
    if (r == ARENA_HEIGHT - 1) {
      int lx = (int)game->launcher_x;
      if (lx >= 0 && lx < ARENA_WIDTH) {
        row_chars[lx] = '^';
      }
    }

    // Now emit the row content with colors
    if (r < BRICK_ROWS) {
      // Brick row
      for (int c = 0; c < BRICK_COLS; c++) {
        const Brick *brk = &game->grid[r][c];
        int start_x = c * BRICK_WIDTH;

        // Check if any ball or laser is in this cell
        bool has_ball = false;
        int ball_sub_x = -1;
        for (int i = 0; i < BRICK_WIDTH; i++) {
          if (row_chars[start_x + i] == 'o') {
            has_ball = true;
            ball_sub_x = i;
            break;
          }
        }

        if (has_ball) {
          // Render ball
          char sub[7] = "      ";
          sub[ball_sub_x] = 'o';
          offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s%s%s", COLOR_BALL, sub, COLOR_RESET);
        } else if (brk->type == BRICK_NORMAL && brk->hp > 0) {
          const char *col = get_brick_color(brk->hp, brk->max_hp);
          if (brk->flash_timer > 0) {
            col = "\033[1;97;47m";
          }
          char b_str[16];
          if (brk->hp >= 1000) {
            snprintf(b_str, sizeof(b_str), "[%4d]", brk->hp);
          } else if (brk->hp >= 100) {
            snprintf(b_str, sizeof(b_str), "[%3d ]", brk->hp);
          } else {
            snprintf(b_str, sizeof(b_str), "[ %2d ]", brk->hp);
          }
          offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s%s%s", col, b_str, COLOR_RESET);
        } else if (brk->type == BRICK_ADD_BALL) {
          offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s  ⦿   %s", COLOR_ORB, COLOR_RESET);
        } else if (brk->type == BRICK_BOMB) {
          offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s  💣  %s", COLOR_BOMB, COLOR_RESET);
        } else if (brk->type == BRICK_LASER_ROW) {
          offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s  ══  %s", COLOR_LASER, COLOR_RESET);
        } else if (brk->type == BRICK_LASER_COL) {
          offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s  ║║  %s", COLOR_LASER, COLOR_RESET);
        } else {
          // Empty or laser dots
          char sub[7];
          memcpy(sub, row_chars + start_x, 6);
          sub[6] = '\0';
          // Highlight laser guide dots
          for (int k = 0; k < 6; k++) {
            if (sub[k] == '.') {
              offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s·%s", COLOR_GUIDE, COLOR_RESET);
            } else {
              offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%c", sub[k]);
            }
          }
        }
      }
    } else {
      // Below brick grid (Flight and Ground rows)
      if (r == DANGER_ROW) {
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s", COLOR_DANGER);
      }
      for (int i = 0; i < ARENA_WIDTH; i++) {
        char ch = row_chars[i];
        if (ch == 'o') {
          offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s●%s", COLOR_BALL, COLOR_RESET);
        } else if (ch == '.') {
          offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s·%s", COLOR_GUIDE, COLOR_RESET);
        } else if (ch == '^') {
          offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s▲%s", COLOR_VALUE, COLOR_RESET);
        } else if (r == DANGER_ROW) {
          offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%c", ch);
        } else {
          offset += snprintf(buffer + offset, sizeof(buffer) - offset, " ");
        }
      }
      if (r == DANGER_ROW) {
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s", COLOR_RESET);
      }
    }

    // Close Arena row and append Sidebar line
    const char *s_line = (side_idx < 36) ? side[side_idx++] : "";
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s║%s   %s\033[K\n", COLOR_ARENA_BORDER, COLOR_RESET, s_line);
  }

  // Render Arena Bottom Border (Line 35) side-by-side with Sidebar Bottom Border
  const char *s_bottom = (side_idx < 36) ? side[side_idx++] : "";
  offset += snprintf(
    buffer + offset, sizeof(buffer) - offset,
    "  %s╚════════════════════════════════════════════════╝%s   %s\033[K\n",
    COLOR_ARENA_BORDER, COLOR_RESET, s_bottom
  );

  // Overlays for Game Over
  if (game->phase == PHASE_GAME_OVER) {
    offset += snprintf(
      buffer + offset, sizeof(buffer) - offset,
      "\n  \033[1;38;2;255;70;70;48;2;40;20;20m"
      " ★★★ GAME OVER! BRICKS REACHED DANGER LINE! Final Round: %d | Score: %d [R: New Run | Q: Quit] ★★★ "
      "\033[0m\033[K\n",
      game->round, game->score
    );
  } else {
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "\033[K\n");
  }

  offset += snprintf(buffer + offset, sizeof(buffer) - offset, "\033[J");

  fwrite(buffer, 1, offset, stdout);
  fflush(stdout);
}
