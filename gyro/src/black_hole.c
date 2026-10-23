#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>
#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#else
#include <sys/ioctl.h>
#include <termios.h>
#include <fcntl.h>
#endif

// Vector Math
typedef struct { float x, y, z; } vec3;

static inline vec3 add(vec3 a, vec3 b) { return (vec3){a.x + b.x, a.y + b.y, a.z + b.z}; }
static inline vec3 sub(vec3 a, vec3 b) { return (vec3){a.x - b.x, a.y - b.y, a.z - b.z}; }
static inline vec3 scale(vec3 v, float s) { return (vec3){v.x * s, v.y * s, v.z * s}; }
static inline float dot(vec3 a, vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
static inline vec3 cross(vec3 a, vec3 b) {
  return (vec3){a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
static inline float length(vec3 v) { return sqrtf(dot(v, v)); }
static inline vec3 normalize(vec3 v) {
  float len = length(v);
  return len > 0 ? scale(v, 1.0f / len) : (vec3){0,0,0};
}

// Raymarching State
typedef struct {
  vec3 p;
  vec3 v;
} RayState;

// Geodesic acceleration: a = -1.5 * Rs * |p x v|^2 / r^5 * p
static inline vec3 calc_accel(vec3 p, vec3 v, float Rs) {
  float r2 = dot(p, p);
  if (r2 < 0.0001f) return (vec3){0,0,0};
  vec3 h = cross(p, v);
  float h2 = dot(h, h);
  float r5 = r2 * r2 * sqrtf(r2);
  float coef = -1.5f * Rs * h2 / r5;
  return scale(p, coef);
}

// RK4 Integrator Step
static inline void rk4_step(RayState* state, float dt, float Rs) {
  vec3 p = state->p;
  vec3 v = state->v;

  vec3 k1_v = calc_accel(p, v, Rs);
  vec3 k1_p = v;

  vec3 p2 = add(p, scale(k1_p, dt * 0.5f));
  vec3 v2 = add(v, scale(k1_v, dt * 0.5f));
  vec3 k2_v = calc_accel(p2, v2, Rs);
  vec3 k2_p = v2;

  vec3 p3 = add(p, scale(k2_p, dt * 0.5f));
  vec3 v3 = add(v, scale(k2_v, dt * 0.5f));
  vec3 k3_v = calc_accel(p3, v3, Rs);
  vec3 k3_p = v3;

  vec3 p4 = add(p, scale(k3_p, dt));
  vec3 v4 = add(v, scale(k3_v, dt));
  vec3 k4_v = calc_accel(p4, v4, Rs);
  vec3 k4_p = v4;

  vec3 sum_p = add(add(k1_p, scale(k2_p, 2.0f)), add(scale(k3_p, 2.0f), k4_p));
  vec3 sum_v = add(add(k1_v, scale(k2_v, 2.0f)), add(scale(k3_v, 2.0f), k4_v));

  state->p = add(p, scale(sum_p, dt / 6.0f));
  state->v = add(v, scale(sum_v, dt / 6.0f));
}

// Skybox Background Calculation
static inline vec3 calc_background(vec3 dir, float time) {
  vec3 col = {0.0f, 0.0f, 0.0f};
  
  // Slowly rotate the skybox to simulate movement
  float t_rot = time * 0.1f;
  float cos_t = cosf(t_rot);
  float sin_t = sinf(t_rot);
  vec3 rot_dir = {
    dir.x * cos_t - dir.z * sin_t,
    dir.y,
    dir.x * sin_t + dir.z * cos_t
  };
  dir = rot_dir;
  
  // Galactic band (Milky way style)
  float band = expf(-powf(fabsf(dir.y) * 4.0f, 2.0f));
  float noise = (sinf(10.0f * dir.x) * cosf(10.0f * dir.z) * 0.5f + 0.5f);
  col.x += band * noise * 20.0f; // R
  col.y += band * noise * 30.0f; // G
  col.z += band * 50.0f;         // B
  
  // Celestial Grid
  float phi = atan2f(dir.z, dir.x);
  float theta = asinf(dir.y);
  float grid_size = 0.2f;
  float line_width = 0.01f;
  if (fabsf(fmodf(phi + grid_size*0.5f, grid_size) - grid_size*0.5f) < line_width || 
    fabsf(fmodf(theta + grid_size*0.5f, grid_size) - grid_size*0.5f) < line_width) {
    col.x += 20.0f;
    col.y += 20.0f;
    col.z += 40.0f;
  }
  
  // Clamp
  if (col.x > 255.0f) col.x = 255.0f;
  if (col.y > 255.0f) col.y = 255.0f;
  if (col.z > 255.0f) col.z = 255.0f;
  
  return col;
}

// Map Intensity to TrueColor and ASCII
static void apply_color(float intensity, vec3 bg_color, char* out_buf) {
  int bg_r = (int)bg_color.x;
  int bg_g = (int)bg_color.y;
  int bg_b = (int)bg_color.z;

  if (intensity < 0.01f) {
    sprintf(out_buf, "\x1b[48;2;%d;%d;%dm \x1b[0m", bg_r, bg_g, bg_b);
    return;
  }
  
  // ASCII ramp
  const char* ramp = " .,-~:;=!*#$@";
  int num_chars = (int)strlen(ramp);
  
  // Scale intensity for visual effect
  float visual_i = intensity * 1.5f;
  int char_idx = (int)(visual_i * (num_chars - 1) / 3.0f);
  if (char_idx < 0) char_idx = 0;
  if (char_idx >= num_chars) char_idx = num_chars - 1;
  char c = ramp[char_idx];

  // Color gradient mapping
  int r = 0, g = 0, b = 0;
  if (visual_i < 0.5f) {
    r = (int)(visual_i * 2.0f * 255.0f); // Black to Red
  } else if (visual_i < 1.5f) {
    r = 255;
    g = (int)((visual_i - 0.5f) * 255.0f); // Red to Yellow
  } else if (visual_i < 2.5f) {
    r = 255;
    g = 255;
    b = (int)((visual_i - 1.5f) * 255.0f); // Yellow to White
  } else {
    r = 255 - (int)((visual_i - 2.5f) * 255.0f); // White to Cyan/Blue
    if (r < 0) r = 0;
    g = 255;
    b = 255;
  }
  
  if (r > 255) r = 255;
  if (g > 255) g = 255;
  if (b > 255) b = 255;

  sprintf(out_buf, "\x1b[38;2;%d;%d;%dm\x1b[48;2;%d;%d;%dm%c\x1b[0m", r, g, b, bg_r, bg_g, bg_b, c);
}

static int raw_term_w = 120, raw_term_h = 30;
static int margin_x = 0;

void get_terminal_size(int *width, int *height) {
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
  struct winsize w;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_col > 0 && w.ws_row > 0) {
    raw_term_w = w.ws_col;
    raw_term_h = w.ws_row - 1;
  } else {
    raw_term_w = 120;
    raw_term_h = 30;
  }
#endif
  if (raw_term_w < 40) raw_term_w = 40;
  if (raw_term_h < 15) raw_term_h = 15;

  *width = (int)(raw_term_w * 0.95f);
  *height = (int)(raw_term_h * 0.95f);
  margin_x = (raw_term_w - *width) / 2;
  if (margin_x < 0) margin_x = 0;
}

// Camera State (Zero-roll Turntable Orbit)
static float camera_dist = 12.0f;
static float camera_yaw = 0.0f;
static float camera_pitch = 0.2f;

void zoom_camera(float delta) {
  camera_dist += delta;
  if (camera_dist < 3.0f) camera_dist = 3.0f;
  if (camera_dist > 40.0f) camera_dist = 40.0f;
}

// Mouse input state
static int mouse_x = -1, mouse_y = -1;
static bool left_down_prev = false;
static bool mid_down_prev = false;
static bool is_mouse_dragging = false;
static int drag_start_x = -1, drag_start_y = -1;
static int last_drag_x = -1, last_drag_y = -1;
static int last_mid_x = -1, last_mid_y = -1;

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
  
  int width = 0, height = 0;
  char *renderBuffer = NULL;
  char **rowBuffers = NULL;

  float Rs = 1.0f; // Schwarzschild radius
  float R_inner = 2.6f * Rs;
  float R_outer = 8.0f * Rs;
  
  float t = 0;

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
          if (key == 'w' || key == 'W' || vk == VK_UP) {
            camera_pitch += 0.08f;
            if (camera_pitch > 1.5533f) camera_pitch = 1.5533f;
          }
          if (key == 's' || key == 'S' || vk == VK_DOWN) {
            camera_pitch -= 0.08f;
            if (camera_pitch < -1.5533f) camera_pitch = -1.5533f;
          }
          if (key == 'a' || key == 'A' || vk == VK_LEFT) camera_yaw -= 0.08f;
          if (key == 'd' || key == 'D' || vk == VK_RIGHT) camera_yaw += 0.08f;
          if (key == '=' || key == '+') zoom_camera(-0.6f);
          if (key == '-' || key == '_') zoom_camera(0.6f);
        } else if (ir[i].EventType == MOUSE_EVENT) {
          MOUSE_EVENT_RECORD mer = ir[i].Event.MouseEvent;
          int cur_x = mer.dwMousePosition.X - margin_x;
          int cur_y = mer.dwMousePosition.Y;
          mouse_x = cur_x;
          mouse_y = cur_y;

          if (mer.dwEventFlags & MOUSE_WHEELED) {
            short wheel_delta = (short)HIWORD(mer.dwButtonState);
            if (wheel_delta > 0) zoom_camera(-0.6f);
            else if (wheel_delta < 0) zoom_camera(0.6f);
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
                camera_yaw -= (float)dx * 0.0075f;
                camera_pitch += (float)dy * 0.0125f;
                if (camera_pitch > 1.5533f) camera_pitch = 1.5533f;
                if (camera_pitch < -1.5533f) camera_pitch = -1.5533f;
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
                camera_yaw -= (float)dx * 0.015f;
                camera_pitch += (float)dy * 0.025f;
                if (camera_pitch > 1.5533f) camera_pitch = 1.5533f;
                if (camera_pitch < -1.5533f) camera_pitch = -1.5533f;
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
            if (b == 64) zoom_camera(-0.6f);
            else if (b == 65) zoom_camera(0.6f);
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
                  camera_yaw -= (float)dx * 0.015f;
                  camera_pitch += (float)dy * 0.025f;
                  if (camera_pitch > 1.5533f) camera_pitch = 1.5533f;
                  if (camera_pitch < -1.5533f) camera_pitch = -1.5533f;
                  last_drag_x = cur_x; last_drag_y = cur_y;
                }
              }
            } else if (m_state == 'm') {
              if (b == 0) { is_mouse_dragging = false; left_down_prev = false; }
            }
          } else if (next2 == 'A') { camera_pitch += 0.08f; if (camera_pitch > 1.5533f) camera_pitch = 1.5533f; }
          else if (next2 == 'B') { camera_pitch -= 0.08f; if (camera_pitch < -1.5533f) camera_pitch = -1.5533f; }
          else if (next2 == 'C') camera_yaw += 0.08f;
          else if (next2 == 'D') camera_yaw -= 0.08f;
        }
      } else {
        if (ch == 'q' || ch == 'Q') running = false;
        if (ch == 'w' || ch == 'W') { camera_pitch += 0.08f; if (camera_pitch > 1.5533f) camera_pitch = 1.5533f; }
        if (ch == 's' || ch == 'S') { camera_pitch -= 0.08f; if (camera_pitch < -1.5533f) camera_pitch = -1.5533f; }
        if (ch == 'a' || ch == 'A') camera_yaw -= 0.08f;
        if (ch == 'd' || ch == 'D') camera_yaw += 0.08f;
        if (ch == '=' || ch == '+') zoom_camera(-0.6f);
        if (ch == '-' || ch == '_') zoom_camera(0.6f);
      }
    }
#endif

    int new_width, new_height;
    get_terminal_size(&new_width, &new_height);
    
    if (new_width != width || new_height != height) {
      int old_height = height;
      width = new_width;
      height = new_height;
      renderBuffer = (char*)realloc(renderBuffer, height * (width * 64 + margin_x + 2) + 512); 
      if (rowBuffers) {
        for(int i = 0; i < old_height; i++) if (rowBuffers[i]) free(rowBuffers[i]);
        free(rowBuffers);
      }
      rowBuffers = (char**)malloc(height * sizeof(char*));
      for(int i = 0; i < height; i++) rowBuffers[i] = (char*)malloc(width * 64 + 2);
    }

    vec3 cam_pos = {
      camera_dist * cosf(camera_pitch) * sinf(camera_yaw),
      camera_dist * sinf(camera_pitch),
      camera_dist * cosf(camera_pitch) * cosf(camera_yaw)
    };
    
    vec3 forward = normalize(sub((vec3){0,0,0}, cam_pos));
    vec3 world_up = {0, 1, 0};
    vec3 right = normalize(cross(forward, world_up));
    vec3 up = cross(right, forward);

    float fov = 1.0f;
    float aspect = (float)width / (float)(height * 2.0f); // Terminal characters are ~2x as tall as wide

    // Parallelized raymarching loop
    #pragma omp parallel for schedule(dynamic)
    for (int y = 0; y < height; y++) {
      char* r_ptr = rowBuffers[y];
      
      for (int x = 0; x < width; x++) {
        float ndc_x = (2.0f * (x + 0.5f) / (float)width - 1.0f) * aspect * fov;
        float ndc_y = (1.0f - 2.0f * (y + 0.5f) / (float)height) * fov;
        
        vec3 ray_dir = normalize(add(add(forward, scale(right, ndc_x)), scale(up, ndc_y)));
        
        RayState state = {cam_pos, ray_dir};
        float intensity = 0.0f;
        float ray_energy = 1.0f;
        vec3 bg_color = {0, 0, 0};
        
        int max_steps = 800;
        float escape_r2 = 400.0f;
        
        for (int step = 0; step < max_steps; step++) {
          float r = length(state.p);
          if (r < Rs * 1.05f) {
            break; // Absorbed by event horizon
          }
          if (r * r > escape_r2) {
            bg_color = calc_background(normalize(state.v), t);
            bg_color = scale(bg_color, ray_energy);
            break; // Escaped
          }

          // Adaptive step size based on distance to event horizon
          float dist_to_horizon = r - Rs;
          
          float dt = 0.05f * dist_to_horizon;
          if (dt < 0.02f) dt = 0.02f;
          if (dt > 0.5f) dt = 0.5f;
          
          vec3 prev_p = state.p;
          rk4_step(&state, dt, Rs);
          
          // Check disk intersection (crossing y=0)
          if (prev_p.y * state.p.y <= 0.0f) {
            float t_intersect = -prev_p.y / (state.p.y - prev_p.y);
            vec3 hit_p = add(prev_p, scale(sub(state.p, prev_p), t_intersect));
            float hit_r = sqrtf(hit_p.x * hit_p.x + hit_p.z * hit_p.z);
            
            if (hit_r > R_inner && hit_r < R_outer) {
              float v_orb = 1.0f / sqrtf(2.0f * (hit_r - Rs)); 
              vec3 V_fluid = {-hit_p.z / hit_r * v_orb, 0.0f, hit_p.x / hit_r * v_orb};
              
              float dot_v = dot(normalize(state.v), V_fluid);
              float doppler = sqrtf(1.0f - v_orb * v_orb) / (1.0f - dot_v);
              
              float phi = atan2f(hit_p.z, hit_p.x);
              float tex = (sinf(10.0f * hit_r) * 0.5f + 0.5f) * (cosf(4.0f * phi - t * 2.0f) * 0.5f + 0.5f);
              float base_glow = 1.5f / (hit_r - Rs);
              
              float add_intensity = (base_glow + tex * 0.5f) * doppler * doppler * doppler;
              intensity += add_intensity * ray_energy;
              ray_energy *= 0.3f;
              
              if (ray_energy < 0.05f) break;
            }
          }
        }
        
        char cell_buf[64];
        apply_color(intensity, bg_color, cell_buf);
        strcpy(r_ptr, cell_buf);
        r_ptr += strlen(cell_buf);
      }
      strcpy(r_ptr, "\n");
    }
    
    // Concatenate all row buffers sequentially with margin_x
    renderBuffer[0] = '\0';
    char* main_ptr = renderBuffer;
    for (int y = 0; y < height; y++) {
      for (int m = 0; m < margin_x; m++) {
        *main_ptr++ = ' ';
      }
      strcpy(main_ptr, rowBuffers[y]);
      main_ptr += strlen(rowBuffers[y]);
    }

    const char *hud = "  [Drag/WASD] Orbit Camera   [Wheel/+/-] Zoom   [Esc] Quit\n";
    strcpy(main_ptr, hud);
    main_ptr += strlen(hud);
    
    // Reset cursor to top-left
    size_t total_bytes = main_ptr - renderBuffer;
    printf("\x1b[H");
    fwrite(renderBuffer, 1, total_bytes, stdout);
    
    t += 0.05f;
#ifdef _WIN32
    Sleep(8);
#else
    usleep(33000);
#endif
  }
  
#ifdef _WIN32
  SetConsoleMode(hIn, origInMode);
#else
  printf("\033[?1003l\033[?1015l\033[?1006l");
  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  fcntl(STDIN_FILENO, F_SETFL, oldf);
#endif

  printf("\x1b[?25h\x1b[0m\n");
  if (renderBuffer) free(renderBuffer);
  if (rowBuffers) {
    for(int i = 0; i < height; i++) if (rowBuffers[i]) free(rowBuffers[i]);
    free(rowBuffers);
  }

  return 0;
}
