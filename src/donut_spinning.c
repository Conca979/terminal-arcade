#include <math.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>

#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#else
#include <termios.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#endif

static int raw_term_w = 120, raw_term_h = 30;
static int margin_x = 0;

void getTerminalSize(int *w, int *h) {
#ifdef _WIN32
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
  if (GetConsoleScreenBufferInfo(hOut, &csbi)) {
    raw_term_w = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    raw_term_h = csbi.srWindow.Bottom - csbi.srWindow.Top;
  } else {
    raw_term_w = 120;
    raw_term_h = 30;
  }
#else
  struct winsize ws;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0 && ws.ws_row > 0) {
    raw_term_w = ws.ws_col;
    raw_term_h = ws.ws_row - 1;
  } else {
    raw_term_w = 120;
    raw_term_h = 30;
  }
#endif
  if (raw_term_w < 40) raw_term_w = 40;
  if (raw_term_h < 15) raw_term_h = 15;

  *w = (int)(raw_term_w * 0.95f);
  *h = (int)(raw_term_h * 0.95f);
  margin_x = (raw_term_w - *w) / 2;
  if (margin_x < 0) margin_x = 0;
}

static int width = 0;
static int height = 0;

static float *zBuffer = NULL;
static char *buffer = NULL;
static char *renderBuffer = NULL;

// Camera State (Zero-roll Turntable Orbit)
static float camA = 0.35f;  // Pitch
static float camB = -0.45f; // Yaw
static float camDist = 5.0f;
static float spinAngle = 0.0f;

// Mouse input state
static int mouse_x = -1, mouse_y = -1;
static bool left_down_prev = false;
static bool mid_down_prev = false;
static bool is_mouse_dragging = false;
static int drag_start_x = -1, drag_start_y = -1;
static int last_drag_x = -1, last_drag_y = -1;
static int last_mid_x = -1, last_mid_y = -1;

void zoom_camera(float delta) {
  camDist += delta;
  if (camDist < 2.5f) camDist = 2.5f;
  if (camDist > 18.0f) camDist = 18.0f;
}

void apply_camera(float x, float y, float z, float *ox, float *oy, float *oz) {
  // Step 1: World Yaw around invariant Y-axis (turntable orbit - zero roll)
  float x1 = x * cosf(camB) + z * sinf(camB);
  float z1 = -x * sinf(camB) + z * cosf(camB);
  float y1 = y;

  // Step 2: Camera Pitch around local X-axis
  float y2 = y1 * cosf(camA) - z1 * sinf(camA);
  float z2 = y1 * sinf(camA) + z1 * cosf(camA);
  float x2 = x1;

  *ox = x2;
  *oy = y2;
  *oz = z2;
}

int main(void) {
  int paused = 0;
  bool running = true;

#ifdef _WIN32
  HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
  DWORD dwMode = 0;
  GetConsoleMode(hOut, &dwMode);
  SetConsoleMode(hOut, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

  HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
  DWORD origInMode = 0;
  GetConsoleMode(hIn, &origInMode);
  DWORD newInMode = origInMode | ENABLE_EXTENDED_FLAGS | ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT;
  newInMode &= ~ENABLE_QUICK_EDIT_MODE;
  SetConsoleMode(hIn, newInMode);
#else
  struct termios oldt, newt;
  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  int oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
  fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

  printf("\033[?1003h\033[?1015h\033[?1006h");
  fflush(stdout);
#endif

  setvbuf(stdout, NULL, _IONBF, 0);
  printf("\x1b[2J\x1b[?25l");

  while (running) {
#ifdef _WIN32
    INPUT_RECORD ir[32];
    DWORD read_count = 0;
    GetNumberOfConsoleInputEvents(hIn, &read_count);
    if (read_count > 0) {
      ReadConsoleInput(hIn, ir, 32, &read_count);
      for (DWORD i = 0; i < read_count; i++) {
        if (ir[i].EventType == KEY_EVENT && ir[i].Event.KeyEvent.bKeyDown) {
          char key = ir[i].Event.KeyEvent.uChar.AsciiChar;
          WORD vk = ir[i].Event.KeyEvent.wVirtualKeyCode;
          if (key == 27 || vk == VK_ESCAPE) running = false;
          if (key == 'w' || key == 'W') { camA -= 0.08f; if (camA < -1.5533f) camA = -1.5533f; }
          if (key == 's' || key == 'S') { camA += 0.08f; if (camA > 1.5533f) camA = 1.5533f; }
          if (key == 'a' || key == 'A') camB -= 0.08f;
          if (key == 'd' || key == 'D') camB += 0.08f;
          if (key == '=' || key == '+') zoom_camera(-0.5f);
          if (key == '-' || key == '_') zoom_camera(0.5f);
          if (key == ' ') paused = !paused;
        } else if (ir[i].EventType == MOUSE_EVENT) {
          MOUSE_EVENT_RECORD mer = ir[i].Event.MouseEvent;
          int cur_x = mer.dwMousePosition.X - margin_x;
          int cur_y = mer.dwMousePosition.Y;
          mouse_x = cur_x;
          mouse_y = cur_y;

          if (mer.dwEventFlags & MOUSE_WHEELED) {
            short wheel_delta = (short)HIWORD(mer.dwButtonState);
            if (wheel_delta > 0) zoom_camera(-0.5f);
            else if (wheel_delta < 0) zoom_camera(0.5f);
          }

          bool left_down = (mer.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED) != 0;
          bool mid_down = (mer.dwButtonState & FROM_LEFT_2ND_BUTTON_PRESSED) != 0;

          if (left_down) {
            if (!left_down_prev) {
              drag_start_x = cur_x;
              drag_start_y = cur_y;
              last_drag_x = cur_x;
              last_drag_y = cur_y;
              is_mouse_dragging = false;
            } else {
              int dx = cur_x - last_drag_x;
              int dy = cur_y - last_drag_y;
              if (abs(cur_x - drag_start_x) > 1 || abs(cur_y - drag_start_y) > 1) {
                is_mouse_dragging = true;
              }
              if (is_mouse_dragging && (dx != 0 || dy != 0)) {
                camB -= (float)dx * 0.0075f;
                camA += (float)dy * 0.0125f;
                if (camA > 1.5533f) camA = 1.5533f;
                if (camA < -1.5533f) camA = -1.5533f;
                last_drag_x = cur_x;
                last_drag_y = cur_y;
              }
            }
          } else if (left_down_prev) {
            is_mouse_dragging = false;
          }

          if (mid_down) {
            if (!mid_down_prev) {
              last_mid_x = cur_x;
              last_mid_y = cur_y;
            } else {
              int dx = cur_x - last_mid_x;
              int dy = cur_y - last_mid_y;
              if (dx != 0 || dy != 0) {
                camB -= (float)dx * 0.015f;
                camA += (float)dy * 0.025f;
                if (camA > 1.5533f) camA = 1.5533f;
                if (camA < -1.5533f) camA = -1.5533f;
                last_mid_x = cur_x;
                last_mid_y = cur_y;
              }
            }
          }

          left_down_prev = left_down;
          mid_down_prev = mid_down;
        }
      }
    }
#else
    int ch;
    while ((ch = getchar()) != EOF) {
      if (ch == 27) {
        int next1 = getchar();
        if (next1 == EOF) { running = false; break; }
        if (next1 == '[') {
          int next2 = getchar();
          if (next2 == '<') {
            int b = 0, mx = 0, my = 0;
            char m_state = 0;
            scanf("%d;%d;%d%c", &b, &mx, &my, &m_state);
            int cur_x = mx - 1 - margin_x;
            int cur_y = my - 1;
            mouse_x = cur_x;
            mouse_y = cur_y;
            if (b == 64) zoom_camera(-0.5f);
            else if (b == 65) zoom_camera(0.5f);
            else if (m_state == 'M') {
              if (b == 0) {
                drag_start_x = cur_x; drag_start_y = cur_y;
                last_drag_x = cur_x; last_drag_y = cur_y;
                is_mouse_dragging = false;
                left_down_prev = true;
              } else if (b == 32) {
                int dx = cur_x - last_drag_x;
                int dy = cur_y - last_drag_y;
                if (abs(cur_x - drag_start_x) > 1 || abs(cur_y - drag_start_y) > 1) is_mouse_dragging = true;
                if (is_mouse_dragging && (dx != 0 || dy != 0)) {
                  camB -= (float)dx * 0.015f;
                  camA += (float)dy * 0.025f;
                  if (camA > 1.5533f) camA = 1.5533f;
                  if (camA < -1.5533f) camA = -1.5533f;
                  last_drag_x = cur_x; last_drag_y = cur_y;
                }
              }
            } else if (m_state == 'm') {
              if (b == 0) { is_mouse_dragging = false; left_down_prev = false; }
            }
          }
        }
      } else {
        if (ch == 'q' || ch == 'Q') running = false;
        if (ch == 'w' || ch == 'W') { camA -= 0.08f; if (camA < -1.5533f) camA = -1.5533f; }
        if (ch == 's' || ch == 'S') { camA += 0.08f; if (camA > 1.5533f) camA = 1.5533f; }
        if (ch == 'a' || ch == 'A') camB -= 0.08f;
        if (ch == 'd' || ch == 'D') camB += 0.08f;
        if (ch == '=' || ch == '+') zoom_camera(-0.5f);
        if (ch == '-' || ch == '_') zoom_camera(0.5f);
        if (ch == ' ') paused = !paused;
      }
    }
#endif

    if (!paused) {
      spinAngle += 0.03f;
    }

    int new_w, new_h;
    getTerminalSize(&new_w, &new_h);
    if (new_w != width || new_h != height) {
      width = new_w;
      height = new_h;
      zBuffer = (float *)realloc(zBuffer, width * height * sizeof(float));
      buffer = (char *)realloc(buffer, width * height * sizeof(char));
      renderBuffer = (char *)realloc(renderBuffer, (width + margin_x + 1) * height + 512);
      printf("\x1b[2J");
    }

    memset(buffer, ' ', width * height);
    memset(zBuffer, 0, width * height * sizeof(float));

    float cosS = cosf(spinAngle), sinS = sinf(spinAngle);
    float K1 = height * 0.68f;

    for (float theta = 0; theta < 6.28318f; theta += 0.035f) {
      float costheta = cosf(theta), sintheta = sinf(theta);

      for (float phi = 0; phi < 6.28318f; phi += 0.015f) {
        float cosphi = cosf(phi), sinphi = sinf(phi);

        float h = costheta + 2.0f; // R2 + R1*costheta (R2=2, R1=1)

        // Point on torus
        float px = h * cosphi;
        float py = sintheta;
        float pz = h * sinphi;

        // Normal on torus
        float nx = costheta * cosphi;
        float ny = sintheta;
        float nz = costheta * sinphi;

        // Apply local spin around X/Z axis to tumble
        float sx = px * cosS + pz * sinS;
        float sz = -px * sinS + pz * cosS;
        float sy = py;

        float snx = nx * cosS + nz * sinS;
        float snz = -nx * sinS + nz * cosS;
        float sny = ny;

        // Apply turntable camera
        float cx, cy, cz;
        apply_camera(sx, sy, sz, &cx, &cy, &cz);

        float cnx, cny, cnz;
        apply_camera(snx, sny, snz, &cnx, &cny, &cnz);

        cz += camDist;
        if (cz <= 0.1f) continue;

        float ooz = 1.0f / cz;
        int xp = (int)(width / 2.0f + K1 * 2.0f * ooz * cx);
        int yp = (int)(height / 2.0f + K1 * ooz * cy);

        // Directional lighting from top-back (0, 1, -1) in camera space
        float LdotN = (cny - cnz) * 0.7071f;

        if (xp >= 0 && xp < width && yp >= 0 && yp < height) {
          int idx = xp + yp * width;
          if (ooz > zBuffer[idx]) {
            zBuffer[idx] = ooz;
            if (LdotN > 0.0f) {
              int luminance = (int)(LdotN * 11.0f);
              if (luminance > 11) luminance = 11;
              buffer[idx] = ".,-~:;=!*#$@"[luminance];
            } else {
              buffer[idx] = '.';
            }
          }
        }
      }
    }

    int p = 0;
    renderBuffer[p++] = '\x1b';
    renderBuffer[p++] = '[';
    renderBuffer[p++] = 'H';

    for (int y = 0; y < height; y++) {
      for (int m = 0; m < margin_x; m++) {
        renderBuffer[p++] = ' ';
      }
      memcpy(&renderBuffer[p], &buffer[y * width], width);
      p += width;
      renderBuffer[p++] = '\n';
    }

    const char *hud = "  [Drag/WASD] Orbit   [Wheel/+/-] Zoom   [Space] Pause   [Esc] Quit\n";
    int hud_len = (int)strlen(hud);
    memcpy(&renderBuffer[p], hud, hud_len);
    p += hud_len;

    fwrite(renderBuffer, 1, p, stdout);
    fflush(stdout);

    usleep(16000);
  }

#ifdef _WIN32
  SetConsoleMode(hIn, origInMode);
#else
  printf("\033[?1003l\033[?1015l\033[?1006l");
  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  fcntl(STDIN_FILENO, F_SETFL, oldf);
#endif

  printf("\x1b[?25h\x1b[0m\n");
  free(zBuffer);
  free(buffer);
  free(renderBuffer);
  return 0;
}