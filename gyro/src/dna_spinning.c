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

static float angleX = 0.3f;
static float angleY = 0.0f;
static float angleZ = 0.0f;
static float camDist = 25.0f;
static float K1 = 40.0f;

static float cosX, sinX, cosY, sinY, cosZ, sinZ;

void zoom_camera(float delta) {
  camDist += delta;
  if (camDist < 5.0f) camDist = 5.0f;
  if (camDist > 80.0f) camDist = 80.0f;
}

// Mouse input state
static int mouse_x = -1, mouse_y = -1;
static bool left_down_prev = false;
static bool mid_down_prev = false;
static bool is_mouse_dragging = false;
static int drag_start_x = -1, drag_start_y = -1;
static int last_drag_x = -1, last_drag_y = -1;
static int last_mid_x = -1, last_mid_y = -1;

void drawPoint(float x, float y, float z, char ch) {
  // Rotate X
  float ty = y * cosX - z * sinX;
  float tz = y * sinX + z * cosX;
  y = ty; z = tz;
  
  // Rotate Y
  float tx = x * cosY + z * sinY;
  tz = -x * sinY + z * cosY;
  x = tx; z = tz;
  
  // Rotate Z
  tx = x * cosZ - y * sinZ;
  ty = x * sinZ + y * cosZ;
  x = tx; y = ty;
  
  z += camDist;
  if (z <= 0.1f) return; 
  
  float ooz = 1.0f / z;
  int xp = (int)(width / 2.0f + K1 * ooz * x * 2.0f);
  int yp = (int)(height / 2.0f + K1 * ooz * y);
  
  int idx = xp + yp * width;
  if (xp >= 0 && xp < width && yp >= 0 && yp < height) {
    if (ooz > zBuffer[idx]) {
      zBuffer[idx] = ooz;
      buffer[idx] = ch;
    }
  }
}

void drawLine(float x1, float y1, float z1, float x2, float y2, float z2, char ch) {
  int steps = 15;
  for (int i = 0; i <= steps; i++) {
    float t = (float)i / steps;
    float x = x1 + (x2 - x1) * t;
    float y = y1 + (y2 - y1) * t;
    float z = z1 + (z2 - z1) * t;
    drawPoint(x, y, z, ch);
  }
}

int main(void) {
  int paused = 0;
  bool running = true;
  float L = 24.0f;       // Total length of the DNA
  float R = 4.0f;        // Radius of the helix
  float turnRate = 0.6f; // Tightness
  float PI = 3.14159265f;

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
          if (key == 'w' || key == 'W' || vk == 'W' || vk == VK_UP) angleX -= 0.15f;
          if (key == 's' || key == 'S' || vk == 'S' || vk == VK_DOWN) angleX += 0.15f;
          if (key == 'a' || key == 'A' || vk == 'A' || vk == VK_LEFT) angleY -= 0.15f;
          if (key == 'd' || key == 'D' || vk == 'D' || vk == VK_RIGHT) angleY += 0.15f;
          if (key == 'q' || key == 'Q' || vk == 'Q') angleZ -= 0.15f;
          if (key == 'e' || key == 'E' || vk == 'E') angleZ += 0.15f;
          if (key == '=' || key == '+' || vk == VK_OEM_PLUS || vk == VK_ADD) zoom_camera(-2.0f);
          if (key == '-' || key == '_' || vk == VK_OEM_MINUS || vk == VK_SUBTRACT) zoom_camera(2.0f);
          if (key == ' ' || vk == VK_SPACE) paused = !paused;
        } else if (ir[i].EventType == MOUSE_EVENT) {
          MOUSE_EVENT_RECORD mer = ir[i].Event.MouseEvent;
          int cur_x = mer.dwMousePosition.X - margin_x;
          int cur_y = mer.dwMousePosition.Y;
          mouse_x = cur_x;
          mouse_y = cur_y;

          if (mer.dwEventFlags & MOUSE_WHEELED) {
            short wheel_delta = (short)HIWORD(mer.dwButtonState);
            if (wheel_delta > 0) zoom_camera(-2.0f);
            else if (wheel_delta < 0) zoom_camera(2.0f);
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
                angleY += (float)dx * 0.01f;
                angleX -= (float)dy * 0.01f;
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
                angleY += (float)dx * 0.02f;
                angleX -= (float)dy * 0.02f;
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
            if (b == 64) zoom_camera(-2.0f);
            else if (b == 65) zoom_camera(2.0f);
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
                  angleY += (float)dx * 0.01f;
                  angleX -= (float)dy * 0.01f;
                  last_drag_x = cur_x; last_drag_y = cur_y;
                }
              }
            } else if (m_state == 'm') {
              if (b == 0) { is_mouse_dragging = false; left_down_prev = false; }
            }
          } else if (next2 == 'A') { angleX -= 0.15f; }
          else if (next2 == 'B') { angleX += 0.15f; }
          else if (next2 == 'C') { angleY += 0.15f; }
          else if (next2 == 'D') { angleY -= 0.15f; }
        }
      } else {
        if (ch == 'w' || ch == 'W') angleX -= 0.15f;
        if (ch == 's' || ch == 'S') angleX += 0.15f;
        if (ch == 'a' || ch == 'A') angleY -= 0.15f;
        if (ch == 'd' || ch == 'D') angleY += 0.15f;
        if (ch == 'q' || ch == 'Q') angleZ -= 0.15f;
        if (ch == 'e' || ch == 'E') angleZ += 0.15f;
        if (ch == '=' || ch == '+') zoom_camera(-2.0f);
        if (ch == '-' || ch == '_') zoom_camera(2.0f);
        if (ch == ' ') paused = !paused;
      }
    }
#endif

    if (!paused) {
      angleY += 0.05f; // Auto rotate
    }

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
    memset(zBuffer, 0, width * height * sizeof(float));

    cosX = cosf(angleX); sinX = sinf(angleX);
    cosY = cosf(angleY); sinY = sinf(angleY);
    cosZ = cosf(angleZ); sinZ = sinf(angleZ);

    // Draw the two helix strands
    for (float y = -L / 2.0f; y <= L / 2.0f; y += 0.05f) {
      float theta = y * turnRate;
      drawPoint(R * cosf(theta), y, R * sinf(theta), '@');
      drawPoint(R * cosf(theta + PI), y, R * sinf(theta + PI), '#');
    }

    // Draw the connecting rungs (base pairs)
    for (float y = -L / 2.0f + 1.0f; y <= L / 2.0f - 1.0f; y += 1.5f) {
      float theta = y * turnRate;
      float x1 = R * cosf(theta);
      float z1 = R * sinf(theta);
      float x2 = R * cosf(theta + PI);
      float z2 = R * sinf(theta + PI);
      drawLine(x1, y, z1, x2, y, z2, '=');
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

    const char *hud = "  [Drag/WASD/Arrows] Orbit   [Q/E] Roll   [Wheel/+/-] Zoom   [Space] Pause   [Esc] Quit\n";
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
