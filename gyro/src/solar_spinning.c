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

// Camera controls (Zero-roll Turntable View)
static float camYaw = 0.0f;
static float camPitch = -0.5f; // Negative = Look down at the planets
static float camX = 0.0f;
static float camY = 30.0f;
static float camZ = -70.0f;

// Simulation controls
static float timeScale = 1.0f;
static int paused = 0;

static float K1 = 40.0f; // Projection scale

// Mouse input state
static int mouse_x = -1, mouse_y = -1;
static bool left_down_prev = false;
static bool mid_down_prev = false;
static bool is_mouse_dragging = false;
static int drag_start_x = -1, drag_start_y = -1;
static int last_drag_x = -1, last_drag_y = -1;
static int last_mid_x = -1, last_mid_y = -1;

void fly_camera(float delta) {
  float fwdX = sinf(camYaw) * cosf(camPitch);
  float fwdY = sinf(camPitch);
  float fwdZ = cosf(camYaw) * cosf(camPitch);
  camX += fwdX * delta;
  camY += fwdY * delta;
  camZ += fwdZ * delta;
}

typedef struct {
  char name[32];
  float radius;
  float orbit_distance;
  float orbit_speed;
  float current_angle;
  int parent_index; // -1 for none
  int has_rings;
  float ring_inner;
  float ring_outer;
  float worldX;
  float worldY;
  float worldZ;
  int is_sun;
} CelestialBody;

#define NUM_BODIES 6
static CelestialBody bodies[NUM_BODIES] = {
  {"Sun",     6.0f,  0.0f,   0.0f,   0.0f, -1, 0, 0.0f,  0.0f,  0,0,0, 1},
  {"Earth",   2.0f,  20.0f,  0.02f,  0.0f,  0, 0, 0.0f,  0.0f,  0,0,0, 0},
  {"Moon",    0.8f,  4.0f,   0.1f,   0.0f,  1, 0, 0.0f,  0.0f,  0,0,0, 0},
  {"Mars",    1.5f,  30.0f,  0.015f, 2.0f,  0, 0, 0.0f,  0.0f,  0,0,0, 0},
  {"Jupiter", 4.0f,  50.0f,  0.008f, 4.0f,  0, 0, 0.0f,  0.0f,  0,0,0, 0},
  {"Saturn",  3.5f,  75.0f,  0.005f, 1.0f,  0, 1, 4.5f,  7.0f,  0,0,0, 0}
};

void projectAndDraw(float wx, float wy, float wz, char ch) {
  float cx = wx - camX;
  float cy = wy - camY;
  float cz = wz - camZ;

  float tx = cx * cosf(camYaw) - cz * sinf(camYaw);
  float tz = cx * sinf(camYaw) + cz * cosf(camYaw);

  float ty = cy * cosf(camPitch) - tz * sinf(camPitch);
  float finalZ = cy * sinf(camPitch) + tz * cosf(camPitch);

  if (finalZ > 0.1f) {
    float ooz = 1.0f / finalZ;
    int xp = (int)(width / 2.0f + K1 * tx * ooz * 2.0f);
    int yp = (int)(height / 2.0f - K1 * ty * ooz);

    if (xp >= 0 && xp < width && yp >= 0 && yp < height) {
      int idx = xp + yp * width;
      if (ooz > zBuffer[idx]) {
        zBuffer[idx] = ooz;
        buffer[idx] = ch;
      }
    }
  }
}

void drawSphere(int bodyIdx) {
  CelestialBody* b = &bodies[bodyIdx];
  float r = b->radius;
  
  float Lx = -b->worldX;
  float Ly = -b->worldY;
  float Lz = -b->worldZ;
  float L_len = sqrtf(Lx*Lx + Ly*Ly + Lz*Lz);
  if (L_len > 0.0f) { Lx /= L_len; Ly /= L_len; Lz /= L_len; }

  for (float theta = 0; theta < 6.28f; theta += 0.15f) {
    for (float phi = 0; phi < 3.14f; phi += 0.15f) {
      float sx = r * sinf(phi) * cosf(theta);
      float sy = r * cosf(phi);
      float sz = r * sinf(phi) * sinf(theta);

      float nx = sx / r;
      float ny = sy / r;
      float nz = sz / r;

      char ch;
      if (b->is_sun) {
        ch = '@';
      } else {
        float LdotN = nx * Lx + ny * Ly + nz * Lz;
        if (LdotN > 0.0f) {
          int luminance_index = (int)(LdotN * 11.0f);
          if (luminance_index > 11) luminance_index = 11;
          const char* shades = ".,-~:;=!*#$@";
          ch = shades[luminance_index];
        } else {
          ch = '.';
        }
      }

      projectAndDraw(b->worldX + sx, b->worldY + sy, b->worldZ + sz, ch);
    }
  }

  if (b->has_rings) {
    for (float ring_r = b->ring_inner; ring_r < b->ring_outer; ring_r += 0.3f) {
      for (float ring_theta = 0; ring_theta < 6.28f; ring_theta += 0.05f) {
        float rx = ring_r * cosf(ring_theta);
        float ry = ring_r * sinf(ring_theta) * 0.3f;
        float rz = ring_r * sinf(ring_theta);

        float nx = 0.0f;
        float ny = 1.0f;
        float nz = 0.0f;

        float LdotN = nx * Lx + ny * Ly + nz * Lz;
        if (LdotN < 0.0f) LdotN = -LdotN; 

        char ch = '.';
        if (LdotN > 0.0f) {
          int luminance_index = (int)(LdotN * 8.0f) + 2;
          if (luminance_index > 11) luminance_index = 11;
          const char* shades = ".,-~:;=!*#$@";
          ch = shades[luminance_index];
        }
        
        if (ring_r > 5.5f && ring_r < 5.8f) ch = ' ';

        if (ch != ' ') {
          projectAndDraw(b->worldX + rx, b->worldY + ry, b->worldZ + rz, ch);
        }
      }
    }
  }
}

void updatePhysics(void) {
  for (int i = 0; i < NUM_BODIES; i++) {
    if (!paused) {
      bodies[i].current_angle += bodies[i].orbit_speed * timeScale;
    }

    float localX = bodies[i].orbit_distance * cosf(bodies[i].current_angle);
    float localZ = bodies[i].orbit_distance * sinf(bodies[i].current_angle);
    float localY = 0.0f;

    if (i == 2) localY = 2.0f * sinf(bodies[i].current_angle);
    if (i == 3) localY = -3.0f * cosf(bodies[i].current_angle);

    if (bodies[i].parent_index != -1) {
      bodies[i].worldX = bodies[bodies[i].parent_index].worldX + localX;
      bodies[i].worldY = bodies[bodies[i].parent_index].worldY + localY;
      bodies[i].worldZ = bodies[bodies[i].parent_index].worldZ + localZ;
    } else {
      bodies[i].worldX = localX;
      bodies[i].worldY = localY;
      bodies[i].worldZ = localZ;
    }
  }
}

int main(void) {
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
    float speed = 3.0f;
    float fwdX = sinf(camYaw) * cosf(camPitch);
    float fwdY = sinf(camPitch);
    float fwdZ = cosf(camYaw) * cosf(camPitch);
    float rightX = cosf(camYaw);
    float rightZ = -sinf(camYaw);

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
          if (key == 'w' || key == 'W') { camX += fwdX * speed; camY += fwdY * speed; camZ += fwdZ * speed; }
          if (key == 's' || key == 'S') { camX -= fwdX * speed; camY -= fwdY * speed; camZ -= fwdZ * speed; }
          if (key == 'a' || key == 'A') { camX -= rightX * speed; camZ -= rightZ * speed; }
          if (key == 'd' || key == 'D') { camX += rightX * speed; camZ += rightZ * speed; }
          if (key == 'e' || key == 'E') camY += speed;
          if (key == 'q' || key == 'Q') camY -= speed;
          if (key == 'j' || key == 'J') camYaw -= 0.08f;
          if (key == 'l' || key == 'L') camYaw += 0.08f;
          if (key == 'i' || key == 'I') { camPitch += 0.08f; if (camPitch > 1.5533f) camPitch = 1.5533f; }
          if (key == 'k' || key == 'K') { camPitch -= 0.08f; if (camPitch < -1.5533f) camPitch = -1.5533f; }
          if (key == '[') timeScale *= 0.5f;
          if (key == ']') timeScale *= 2.0f;
          if (key == 'p' || key == 'P') paused = !paused;
        } else if (ir[i].EventType == MOUSE_EVENT) {
          MOUSE_EVENT_RECORD mer = ir[i].Event.MouseEvent;
          int cur_x = mer.dwMousePosition.X - margin_x;
          int cur_y = mer.dwMousePosition.Y;
          mouse_x = cur_x;
          mouse_y = cur_y;

          if (mer.dwEventFlags & MOUSE_WHEELED) {
            short wheel_delta = (short)HIWORD(mer.dwButtonState);
            if (wheel_delta > 0) fly_camera(4.0f);
            else if (wheel_delta < 0) fly_camera(-4.0f);
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
              int delta_x = cur_x - last_drag_x;
              int delta_y = cur_y - last_drag_y;
              if (abs(cur_x - drag_start_x) > 1 || abs(cur_y - drag_start_y) > 1) {
                is_mouse_dragging = true;
              }
              if (is_mouse_dragging && (delta_x != 0 || delta_y != 0)) {
                camYaw -= (float)delta_x * 0.0075f;
                camPitch += (float)delta_y * 0.0125f;
                if (camPitch > 1.5533f) camPitch = 1.5533f;
                if (camPitch < -1.5533f) camPitch = -1.5533f;
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
              int delta_x = cur_x - last_mid_x;
              int delta_y = cur_y - last_mid_y;
              if (delta_x != 0 || delta_y != 0) {
                camYaw -= (float)delta_x * 0.015f;
                camPitch += (float)delta_y * 0.025f;
                if (camPitch > 1.5533f) camPitch = 1.5533f;
                if (camPitch < -1.5533f) camPitch = -1.5533f;
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
            if (b == 64) fly_camera(4.0f);
            else if (b == 65) fly_camera(-4.0f);
            else if (m_state == 'M') {
              if (b == 0) {
                drag_start_x = cur_x; drag_start_y = cur_y;
                last_drag_x = cur_x; last_drag_y = cur_y;
                is_mouse_dragging = false;
                left_down_prev = true;
              } else if (b == 32) {
                int delta_x = cur_x - last_drag_x;
                int delta_y = cur_y - last_drag_y;
                if (abs(cur_x - drag_start_x) > 1 || abs(cur_y - drag_start_y) > 1) is_mouse_dragging = true;
                if (is_mouse_dragging && (delta_x != 0 || delta_y != 0)) {
                  camYaw -= (float)delta_x * 0.015f;
                  camPitch += (float)delta_y * 0.025f;
                  if (camPitch > 1.5533f) camPitch = 1.5533f;
                  if (camPitch < -1.5533f) camPitch = -1.5533f;
                  last_drag_x = cur_x; last_drag_y = cur_y;
                }
              }
            } else if (m_state == 'm') {
              if (b == 0) { is_mouse_dragging = false; left_down_prev = false; }
            }
          }
        }
      } else {
        if (ch == 'q' || ch == 'Q') camY -= speed;
        if (ch == 'e' || ch == 'E') camY += speed;
        if (ch == 'w' || ch == 'W') { camX += fwdX * speed; camY += fwdY * speed; camZ += fwdZ * speed; }
        if (ch == 's' || ch == 'S') { camX -= fwdX * speed; camY -= fwdY * speed; camZ -= fwdZ * speed; }
        if (ch == 'a' || ch == 'A') { camX -= rightX * speed; camZ -= rightZ * speed; }
        if (ch == 'd' || ch == 'D') { camX += rightX * speed; camZ += rightZ * speed; }
        if (ch == 'j' || ch == 'J') camYaw -= 0.08f;
        if (ch == 'l' || ch == 'L') camYaw += 0.08f;
        if (ch == 'i' || ch == 'I') { camPitch += 0.08f; if (camPitch > 1.5533f) camPitch = 1.5533f; }
        if (ch == 'k' || ch == 'K') { camPitch -= 0.08f; if (camPitch < -1.5533f) camPitch = -1.5533f; }
        if (ch == '[') timeScale *= 0.5f;
        if (ch == ']') timeScale *= 2.0f;
        if (ch == 'p' || ch == 'P') paused = !paused;
      }
    }
#endif

    updatePhysics();

    int new_w, new_h;
    getTerminalSize(&new_w, &new_h);
    if (new_w != width || new_h != height) {
      width = new_w;
      height = new_h;
      zBuffer = (float *)realloc(zBuffer, width * height * sizeof(float));
      buffer = (char *)realloc(buffer, width * height * sizeof(char));
      renderBuffer = (char *)realloc(renderBuffer, (width + margin_x + 1) * height + 512);
      K1 = height * 0.9f;
      printf("\x1b[2J");
    }

    memset(buffer, ' ', width * height);
    for (int i = 0; i < width * height; i++) {
      zBuffer[i] = 0.0f;
    }

    // Reference grid
    for (int x = -100; x <= 100; x += 10) {
      for (int z = -100; z <= 100; z += 10) {
        projectAndDraw((float)x, -10.0f, (float)z, '.');
      }
    }

    // Celestial bodies
    for (int i = 0; i < NUM_BODIES; i++) {
      drawSphere(i);
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
    
    p += snprintf(&renderBuffer[p], 256, 
      "  [WASD/QE] Fly   [Drag/IJKL] Look   [Wheel] Fly Fwd/Back   [[ / ]] Speed: %.2fx   [P] Pause   [Esc] Exit\n", 
      timeScale);

    fwrite(renderBuffer, 1, p, stdout);
    fflush(stdout);

    usleep(30000);
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
