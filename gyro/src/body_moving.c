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

static float camX = 0, camY = 0, camZ = 0;
static float camYaw = 0.0f, camPitch = 0.35f;
static float camDist = 35.0f;
static float K1 = 50.0f;

static float robotX = 0, robotY = 0, robotZ = 0;
static float robotYaw = 0.0f;

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
  if (camDist < 10.0f) camDist = 10.0f;
  if (camDist > 100.0f) camDist = 100.0f;
}

void projectWorld(float wx, float wy, float wz, char ch) {
  float cx = wx - camX;
  float cy = wy - camY;
  float cz = wz - camZ;
  
  float sY = sinf(-camYaw), cY = cosf(-camYaw);
  float cx2 = cx * cY + cz * sY;
  float cz2 = -cx * sY + cz * cY;
  cx = cx2; cz = cz2;
  
  float sP = sinf(-camPitch), cP = cosf(-camPitch);
  float cy2 = cy * cP - cz * sP;
  float cz3 = cy * sP + cz * cP;
  cy = cy2; cz = cz3;
  
  if (cz <= 0.1f) return;
  
  float ooz = 1.0f / cz;
  int xp = (int)(width / 2.0f + K1 * ooz * cx * 2.0f);
  int yp = (int)(height / 2.0f - K1 * ooz * cy); // Y inverted for terminal
  
  int idx = xp + yp * width;
  if (xp >= 0 && xp < width && yp >= 0 && yp < height) {
    if (ooz > zBuffer[idx]) {
      zBuffer[idx] = ooz;
      buffer[idx] = ch;
    }
  }
}

void transformAndDrawFast(float lx, float ly, float lz, float jx, float jy, float jz, float sX, float cX, float sRY, float cRY, char ch) {
  // 1. Joint rotation X
  float ty = ly * cX - lz * sX;
  float tz = ly * sX + lz * cX;
  ly = ty; lz = tz;
  
  // 2. Attach to body
  float bx = lx + jx;
  float by = ly + jy;
  float bz = lz + jz;
  
  // 3. Robot rotation (Yaw)
  float rx = bx * cRY + bz * sRY;
  float rz = -bx * sRY + bz * cRY;
  bx = rx; bz = rz;
  
  // 4. World position
  float wx = bx + robotX;
  float wy = by + robotY;
  float wz = bz + robotZ;
  
  projectWorld(wx, wy, wz, ch);
}

void drawBox(
  float bw, float bh, float bd,
  float ox, float oy, float oz, 
  float jx, float jy, float jz, 
  float rotX, float rotY, float rotZ, 
  char ch
) {
  (void)rotY;
  (void)rotZ;
  float sX = sinf(rotX);
  float cX = cosf(rotX);
  float sRY = sinf(robotYaw);
  float cRY = cosf(robotYaw);
  float step = 0.5f;
  for (float x = -bw/2; x <= bw/2; x += step) {
    for (float y = -bh/2; y <= bh/2; y += step) {
      transformAndDrawFast(x + ox, y + oy, -bd/2 + oz, jx, jy, jz, sX, cX, sRY, cRY, ch);
      transformAndDrawFast(x + ox, y + oy, bd/2 + oz, jx, jy, jz, sX, cX, sRY, cRY, ch);
    }
  }
  for (float z = -bd/2; z <= bd/2; z += step) {
    for (float y = -bh/2; y <= bh/2; y += step) {
      transformAndDrawFast(-bw/2 + ox, y + oy, z + oz, jx, jy, jz, sX, cX, sRY, cRY, ch);
      transformAndDrawFast(bw/2 + ox, y + oy, z + oz, jx, jy, jz, sX, cX, sRY, cRY, ch);
    }
  }
  for (float x = -bw/2; x <= bw/2; x += step) {
    for (float z = -bd/2; z <= bd/2; z += step) {
      transformAndDrawFast(x + ox, -bh/2 + oy, z + oz, jx, jy, jz, sX, cX, sRY, cRY, ch);
      transformAndDrawFast(x + ox, bh/2 + oy, z + oz, jx, jy, jz, sX, cX, sRY, cRY, ch);
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
  
  float walkPhase = 0.0f;
  float currentLimbAngle = 0.0f;
  float targetRobotYaw = robotYaw;
  int timeSinceLastMove = 100;
  
  while (running) {
    int isMoving = 0;
    float dz = 0, dx = 0;

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
          
          float moveSpeed = 1.0f;
          if (key == 'w' || key == 'W') dz += moveSpeed;
          if (key == 's' || key == 'S') dz -= moveSpeed;
          if (key == 'a' || key == 'A') dx -= moveSpeed;
          if (key == 'd' || key == 'D') dx += moveSpeed;

          if (key == 'i' || key == 'I') { camPitch += 0.08f; if (camPitch > 1.5533f) camPitch = 1.5533f; }
          if (key == 'k' || key == 'K') { camPitch -= 0.08f; if (camPitch < -0.2f) camPitch = -0.2f; }
          if (key == 'j' || key == 'J') camYaw -= 0.08f;
          if (key == 'l' || key == 'L') camYaw += 0.08f;
          
          if (key == '=' || key == '+') zoom_camera(-2.0f);
          if (key == '-' || key == '_') zoom_camera(2.0f);
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
              int delta_x = cur_x - last_drag_x;
              int delta_y = cur_y - last_drag_y;
              if (abs(cur_x - drag_start_x) > 1 || abs(cur_y - drag_start_y) > 1) {
                is_mouse_dragging = true;
              }
              if (is_mouse_dragging && (delta_x != 0 || delta_y != 0)) {
                camYaw -= (float)delta_x * 0.0075f;
                camPitch += (float)delta_y * 0.0125f;
                if (camPitch > 1.5533f) camPitch = 1.5533f;
                if (camPitch < -0.2f) camPitch = -0.2f;
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
                if (camPitch < -0.2f) camPitch = -0.2f;
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
                int delta_x = cur_x - last_drag_x;
                int delta_y = cur_y - last_drag_y;
                if (abs(cur_x - drag_start_x) > 1 || abs(cur_y - drag_start_y) > 1) is_mouse_dragging = true;
                if (is_mouse_dragging && (delta_x != 0 || delta_y != 0)) {
                  camYaw -= (float)delta_x * 0.015f;
                  camPitch += (float)delta_y * 0.025f;
                  if (camPitch > 1.5533f) camPitch = 1.5533f;
                  if (camPitch < -0.2f) camPitch = -0.2f;
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
        float moveSpeed = 1.0f;
        if (ch == 'w' || ch == 'W') dz += moveSpeed;
        if (ch == 's' || ch == 'S') dz -= moveSpeed;
        if (ch == 'a' || ch == 'A') dx -= moveSpeed;
        if (ch == 'd' || ch == 'D') dx += moveSpeed;
        if (ch == 'i' || ch == 'I') { camPitch += 0.08f; if (camPitch > 1.5533f) camPitch = 1.5533f; }
        if (ch == 'k' || ch == 'K') { camPitch -= 0.08f; if (camPitch < -0.2f) camPitch = -0.2f; }
        if (ch == 'j' || ch == 'J') camYaw -= 0.08f;
        if (ch == 'l' || ch == 'L') camYaw += 0.08f;
        if (ch == '=' || ch == '+') zoom_camera(-2.0f);
        if (ch == '-' || ch == '_') zoom_camera(2.0f);
      }
    }
#endif

    if (dz != 0 || dx != 0) {
      isMoving = 1;
      float fx = sinf(camYaw), fz = cosf(camYaw);
      float rx = cosf(camYaw), rz = -sinf(camYaw);
      
      float worldDx = fx * dz + rx * dx;
      float worldDz = fz * dz + rz * dx;
      
      robotX += worldDx;
      robotZ += worldDz;
      
      targetRobotYaw = atan2f(worldDx, worldDz);
    }

    if (isMoving) timeSinceLastMove = 0;
    else timeSinceLastMove++;

    float targetLimbAngle = 0.0f;
    if (timeSinceLastMove < 3) {
      walkPhase += 0.35f;
      targetLimbAngle = sinf(walkPhase) * 1.2f;
    } else {
      walkPhase = 0.0f;
    }
    
    currentLimbAngle += (targetLimbAngle - currentLimbAngle) * 0.3f;
    
    float diff = targetRobotYaw - robotYaw;
    while (diff > 3.14159f) diff -= 6.28318f;
    while (diff < -3.14159f) diff += 6.28318f;
    robotYaw += diff * 0.15f;

    // Turntable orbital camera placement
    camX = robotX - sinf(camYaw) * camDist * cosf(camPitch);
    camY = robotY + sinf(camPitch) * camDist;
    camZ = robotZ - cosf(camYaw) * camDist * cosf(camPitch);

    int new_w, new_h;
    getTerminalSize(&new_w, &new_h);
    if (new_w != width || new_h != height) {
      width = new_w;
      height = new_h;
      zBuffer = (float *)realloc(zBuffer, width * height * sizeof(float));
      buffer = (char *)realloc(buffer, width * height * sizeof(char));
      renderBuffer = (char *)realloc(renderBuffer, (width + margin_x + 1) * height + 512);
      K1 = height * 1.13f;
      printf("\x1b[2J");
    }

    memset(buffer, ' ', width * height);
    memset(zBuffer, 0, width * height * sizeof(float));

    // Draw Floor Grid
    for (float fx = -60; fx <= 60; fx += 3) {
      for (float fz = -60; fz <= 60; fz += 3) {
        projectWorld(fx, -8, fz, '.');
      }
    }

    // Draw Robot Body
    // Torso (Pivot center)
    drawBox(4, 6, 2,  0, 0, 0,   0, 0, 0,   0, 0, 0, '#');
    // Head 
    drawBox(2.5, 2.5, 2.5,  0, 0, 0,   0, 4.5, 0,   0, 0, 0, '@');
    // Arms (Pivot at shoulder)
    drawBox(1.5, 5, 1.5,  0, -2.5, 0,  -3.5, 2.5, 0,  currentLimbAngle, 0, 0, '=');
    drawBox(1.5, 5, 1.5,  0, -2.5, 0,   3.5, 2.5, 0, -currentLimbAngle, 0, 0, '=');
    // Legs (Pivot at hips)
    drawBox(1.5, 6, 1.5,  0, -3.0, 0,  -1.5, -3, 0,  -currentLimbAngle, 0, 0, ':');
    drawBox(1.5, 6, 1.5,  0, -3.0, 0,   1.5, -3, 0,   currentLimbAngle, 0, 0, ':');

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
    
    const char *hud = "  [WASD] Move Robot   [Drag/IJKL] Orbit Camera   [Wheel/+/-] Zoom   [Esc] Quit\n";
    int hud_len = (int)strlen(hud);
    memcpy(&renderBuffer[p], hud, hud_len);
    p += hud_len;

    fwrite(renderBuffer, 1, p, stdout);
    fflush(stdout);

    usleep(10000);
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
