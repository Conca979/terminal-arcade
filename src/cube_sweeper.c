#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <termios.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#endif

// --------------------------------------------------------
// Platform specific functions
// --------------------------------------------------------

#ifndef _WIN32
long long get_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (((long long)tv.tv_sec)*1000) + (tv.tv_usec/1000);
}
#endif

int raw_term_w = 120, raw_term_h = 30;
int margin_x = 0;

void getTerminalSize(int *w, int *h) {
#ifdef _WIN32
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
  if (GetConsoleScreenBufferInfo(hOut, &csbi)) {
    raw_term_w = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    raw_term_h = csbi.srWindow.Bottom - csbi.srWindow.Top;
  } else {
    raw_term_w = 120; raw_term_h = 30;
  }
#else
  struct winsize ws;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0 && ws.ws_row > 0) {
    raw_term_w = ws.ws_col;
    raw_term_h = ws.ws_row - 1;
  } else {
    raw_term_w = 120; raw_term_h = 30;
  }
#endif
  if (raw_term_w < 40) raw_term_w = 40;
  if (raw_term_h < 15) raw_term_h = 15;
  *w = (int)(raw_term_w * 0.95f);
  *h = (int)(raw_term_h * 0.95f);
  margin_x = (raw_term_w - *w) / 2;
  if (margin_x < 0) margin_x = 0;
}

// --------------------------------------------------------
// Vector & Math
// --------------------------------------------------------

typedef struct { float x, y, z; } vec3;

float length(vec3 v) { return sqrtf(v.x*v.x + v.y*v.y + v.z*v.z); }
float distance(vec3 a, vec3 b) {
    return sqrtf((a.x-b.x)*(a.x-b.x) + (a.y-b.y)*(a.y-b.y) + (a.z-b.z)*(a.z-b.z));
}
vec3 add(vec3 a, vec3 b) { return (vec3){a.x+b.x, a.y+b.y, a.z+b.z}; }
vec3 sub(vec3 a, vec3 b) { return (vec3){a.x-b.x, a.y-b.y, a.z-b.z}; }
vec3 scale(vec3 v, float s) { return (vec3){v.x*s, v.y*s, v.z*s}; }

// --------------------------------------------------------
// Game State
// --------------------------------------------------------

typedef enum {
    MODE_SURFACE = 0,
    MODE_BLOCK = 1
} GameMode;

GameMode current_mode = MODE_SURFACE;

typedef struct Tile {
    int id;
    int face, r, c;
    vec3 center;
    vec3 normal;
    vec3 v[4]; // 4 corners for rendering
    bool is_mine;
    bool is_revealed;
    bool is_flagged;
    int adjacent_mines;
    int num_neighbors;
    struct Tile* neighbors[8];
} Tile;

typedef struct Block {
    int id;
    int x, y, z;
    vec3 center;
    bool is_mine;
    bool is_revealed;
    bool is_flagged;
    int adjacent_mines;
    int num_neighbors;
    int neighbors[26];
} Block;

Tile* tiles = NULL;
int T = 0;

Block* blocks = NULL;
int total_blocks = 0;

int H, W, D;
int M = 0; // num mines
bool first_click = true;
bool game_over = false;
bool game_won = false;

float camA = 0.5f, camB = -0.5f;
float distanceFromCam = 25.0f;
float K1 = 40.0f;

int width = 0, height = 0;
float *zBuffer = NULL;
char *char_buffer = NULL;
int *color_buffer = NULL;
int *id_buffer = NULL;
int *face_buffer = NULL;
char *renderBuffer = NULL;

// Input state
int mouse_x = -1, mouse_y = -1;
long long last_left_click_time = 0;
bool left_down_prev = false;
bool right_down_prev = false;
bool mid_down_prev = false;
bool is_mouse_dragging = false;
int drag_start_x = -1, drag_start_y = -1;
int last_drag_x = -1, last_drag_y = -1;
int last_mid_x = -1, last_mid_y = -1;

// --------------------------------------------------------
// Rendering Math
// --------------------------------------------------------

void apply_camera(vec3 p, vec3 *out) {
    // Step 1: World Yaw around invariant Y-axis (turntable orbit - zero roll)
    float x1 = p.x * cosf(camB) + p.z * sinf(camB);
    float z1 = -p.x * sinf(camB) + p.z * cosf(camB);
    float y1 = p.y;
    
    // Step 2: Camera Pitch around local X-axis
    float y2 = y1 * cosf(camA) - z1 * sinf(camA);
    float z2 = y1 * sinf(camA) + z1 * cosf(camA);
    float x2 = x1;
    
    out->x = x2; out->y = y2; out->z = z2;
}

float edgeFunction(float ax, float ay, float bx, float by, float px, float py) {
    return (px - ax) * (by - ay) - (py - ay) * (bx - ax);
}

void rasterize_triangle(vec3 v0, vec3 v1, vec3 v2, int color, char ch, int tile_id, int face_id) {
    vec3 p0, p1, p2;
    apply_camera(v0, &p0);
    apply_camera(v1, &p1);
    apply_camera(v2, &p2);
    
    p0.z += distanceFromCam; p1.z += distanceFromCam; p2.z += distanceFromCam;
    if (p0.z <= 0.1f || p1.z <= 0.1f || p2.z <= 0.1f) return;
    
    float ooz0 = 1.0f / p0.z; float ooz1 = 1.0f / p1.z; float ooz2 = 1.0f / p2.z;
    
    float center_x = width * 0.5f;
    float center_y = height * 0.5f;
    
    float x0 = center_x + (p0.x * ooz0) * K1 * 2.0f; float y0 = center_y + (p0.y * ooz0) * K1;
    float x1 = center_x + (p1.x * ooz1) * K1 * 2.0f; float y1 = center_y + (p1.y * ooz1) * K1;
    float x2 = center_x + (p2.x * ooz2) * K1 * 2.0f; float y2 = center_y + (p2.y * ooz2) * K1;
    
    int min_x = (int)fminf(x0, fminf(x1, x2)); int max_x = (int)fmaxf(x0, fmaxf(x1, x2));
    int min_y = (int)fminf(y0, fminf(y1, y2)); int max_y = (int)fmaxf(y0, fmaxf(y1, y2));
    
    if (min_x < 0) min_x = 0; 
    if (max_x >= width) max_x = width - 1;
    if (min_y < 0) min_y = 0; 
    if (max_y >= height) max_y = height - 1;
    
    float area = edgeFunction(x0, y0, x1, y1, x2, y2);
    if (area == 0.0f) return;
    
    for (int y = min_y; y <= max_y; y++) {
        for (int x = min_x; x <= max_x; x++) {
            float w0 = edgeFunction(x1, y1, x2, y2, x, y);
            float w1 = edgeFunction(x2, y2, x0, y0, x, y);
            float w2 = edgeFunction(x0, y0, x1, y1, x, y);
            if ((w0 >= 0 && w1 >= 0 && w2 >= 0) || (w0 <= 0 && w1 <= 0 && w2 <= 0)) {
                w0 /= area; w1 /= area; w2 /= area;
                float z = 1.0f / (w0 * ooz0 + w1 * ooz1 + w2 * ooz2);
                int idx = y * width + x;
                if (z < zBuffer[idx]) {
                    zBuffer[idx] = z;
                    color_buffer[idx] = color;
                    char_buffer[idx] = ch;
                    id_buffer[idx] = tile_id;
                    if (face_buffer) face_buffer[idx] = face_id;
                }
            }
        }
    }
}

// --------------------------------------------------------
// Game Logic
// --------------------------------------------------------

void generate_cuboid() {
    H = 3 + rand() % 5;
    W = 3 + rand() % 5;
    D = 3 + rand() % 5;
    
    T = 2*(W*D) + 2*(W*H) + 2*(D*H);
    tiles = (Tile*)malloc(sizeof(Tile) * T);
    
    int id = 0;
    // Face 0: Front
    for(int r=0; r<H; r++) {
        for(int c=0; c<W; c++) {
            Tile* t = &tiles[id];
            t->id = id++; t->face = 0; t->r = r; t->c = c;
            t->center = (vec3){c - (W-1)/2.0f, (H-1)/2.0f - r, D/2.0f};
            t->normal = (vec3){0, 0, 1};
        }
    }
    // Face 1: Back
    for(int r=0; r<H; r++) {
        for(int c=0; c<W; c++) {
            Tile* t = &tiles[id];
            t->id = id++; t->face = 1; t->r = r; t->c = c;
            t->center = (vec3){(W-1)/2.0f - c, (H-1)/2.0f - r, -D/2.0f};
            t->normal = (vec3){0, 0, -1};
        }
    }
    // Face 2: Top
    for(int r=0; r<D; r++) {
        for(int c=0; c<W; c++) {
            Tile* t = &tiles[id];
            t->id = id++; t->face = 2; t->r = r; t->c = c;
            t->center = (vec3){c - (W-1)/2.0f, H/2.0f, (D-1)/2.0f - r};
            t->normal = (vec3){0, 1, 0};
        }
    }
    // Face 3: Bottom
    for(int r=0; r<D; r++) {
        for(int c=0; c<W; c++) {
            Tile* t = &tiles[id];
            t->id = id++; t->face = 3; t->r = r; t->c = c;
            t->center = (vec3){c - (W-1)/2.0f, -H/2.0f, r - (D-1)/2.0f};
            t->normal = (vec3){0, -1, 0};
        }
    }
    // Face 4: Left
    for(int r=0; r<H; r++) {
        for(int c=0; c<D; c++) {
            Tile* t = &tiles[id];
            t->id = id++; t->face = 4; t->r = r; t->c = c;
            t->center = (vec3){-W/2.0f, (H-1)/2.0f - r, c - (D-1)/2.0f};
            t->normal = (vec3){-1, 0, 0};
        }
    }
    // Face 5: Right
    for(int r=0; r<H; r++) {
        for(int c=0; c<D; c++) {
            Tile* t = &tiles[id];
            t->id = id++; t->face = 5; t->r = r; t->c = c;
            t->center = (vec3){W/2.0f, (H-1)/2.0f - r, (D-1)/2.0f - c};
            t->normal = (vec3){1, 0, 0};
        }
    }
    
    // Setup vertices, states, and neighbors
    for(int i=0; i<T; i++) {
        tiles[i].is_mine = false;
        tiles[i].is_revealed = false;
        tiles[i].is_flagged = false;
        tiles[i].adjacent_mines = 0;
        tiles[i].num_neighbors = 0;
        
        vec3 u, v;
        if (tiles[i].normal.y != 0) { u = (vec3){1,0,0}; v = (vec3){0,0,-1}; }
        else if (tiles[i].normal.x != 0) { u = (vec3){0,0,1}; v = (vec3){0, -1, 0}; }
        else { u = (vec3){1,0,0}; v = (vec3){0, -1, 0}; }
        
        if (tiles[i].face == 1 || tiles[i].face == 3 || tiles[i].face == 4) {
            u = scale(u, -1.0f);
        }
        
        float s = 0.5f; // clean 20% gap for distinct black grid lines between tiles
        tiles[i].v[0] = add(add(tiles[i].center, scale(u, -s)), scale(v, -s));
        tiles[i].v[1] = add(add(tiles[i].center, scale(u, s)), scale(v, -s));
        tiles[i].v[2] = add(add(tiles[i].center, scale(u, s)), scale(v, s));
        tiles[i].v[3] = add(add(tiles[i].center, scale(u, -s)), scale(v, s));
    }
    
    // Find neighbors based on 3D distance threshold (d <= 1.45)
    for(int i=0; i<T; i++) {
        for(int j=0; j<T; j++) {
            if (i == j) continue;
            if (distance(tiles[i].center, tiles[j].center) <= 1.45f) {
                tiles[i].neighbors[tiles[i].num_neighbors++] = &tiles[j];
            }
        }
    }
    
    M = (int)(T * 0.206f); // Expert density
    distanceFromCam = fmaxf(H, fmaxf(W, D)) * 1.5f + 10.0f;
}

int get_sweeper_font_bits(char c) {
    switch (c) {
        case '1': return 11415;
        case '2': return 29671;
        case '3': return 29391;
        case '4': return 23497;
        case '5': return 31183;
        case '6': return 14827;
        case '7': return 29330;
        case '8': return 15083;
        case 'F': return 31140;
        case '*': return 21973;
        default: return 0;
    }
}

int get_sweeper_font_bits_3x3(char c) {
    switch (c) {
        case '1': return 146; // 010 010 010
        case '2': return 471; // 111 010 111
        case '3': return 479; // 111 011 111
        case '4': return 377; // 101 111 001
        case '5': return 503; // 111 110 111
        case '6': return 319; // 100 111 111
        case '7': return 457; // 111 001 001
        case '8': return 495; // 111 101 111
        case 'F': return 500; // 111 110 100
        case '*': return 186; // 010 111 010
        default: return 0;
    }
}

void zoom_camera(float delta) {
    float max_dim = fmaxf((float)H, fmaxf((float)W, (float)D));
    float min_dist = max_dim * 0.8f + 2.0f;
    float max_dist = max_dim * 3.5f + 35.0f;
    distanceFromCam += delta;
    if (distanceFromCam < min_dist) distanceFromCam = min_dist;
    if (distanceFromCam > max_dist) distanceFromCam = max_dist;
}

bool verify_surface_board_solvable(int safe_id) {
    int *sim_state = (int*)calloc(T, sizeof(int));
    int *queue = (int*)malloc(sizeof(int) * T);
    int *frontier = (int*)malloc(sizeof(int) * T);
    int q_head = 0, q_tail = 0;
    
    sim_state[safe_id] = 1;
    queue[q_tail++] = safe_id;
    
    while(q_head < q_tail) {
        int cur = queue[q_head++];
        if (tiles[cur].adjacent_mines == 0) {
            for(int j=0; j<tiles[cur].num_neighbors; j++) {
                int nid = tiles[cur].neighbors[j]->id;
                if (sim_state[nid] == 0) {
                    sim_state[nid] = 1;
                    queue[q_tail++] = nid;
                }
            }
        }
    }
    
    bool progress = true;
    while(progress) {
        progress = false;
        
        // Tier 1: Direct counting
        for(int i=0; i<T; i++) {
            if (sim_state[i] != 1 || tiles[i].adjacent_mines == 0) continue;
            
            int hidden_count = 0;
            int flagged_count = 0;
            for(int j=0; j<tiles[i].num_neighbors; j++) {
                int nid = tiles[i].neighbors[j]->id;
                if (sim_state[nid] == 0) hidden_count++;
                else if (sim_state[nid] == 2) flagged_count++;
            }
            
            int needed = tiles[i].adjacent_mines - flagged_count;
            if (hidden_count > 0) {
                if (hidden_count == needed) {
                    for(int j=0; j<tiles[i].num_neighbors; j++) {
                        int nid = tiles[i].neighbors[j]->id;
                        if (sim_state[nid] == 0) {
                            sim_state[nid] = 2;
                            progress = true;
                        }
                    }
                } else if (needed == 0) {
                    for(int j=0; j<tiles[i].num_neighbors; j++) {
                        int nid = tiles[i].neighbors[j]->id;
                        if (sim_state[nid] == 0) {
                            sim_state[nid] = 1;
                            queue[q_tail++] = nid;
                            progress = true;
                        }
                    }
                }
            }
        }
        
        while(q_head < q_tail) {
            int cur = queue[q_head++];
            if (tiles[cur].adjacent_mines == 0) {
                for(int j=0; j<tiles[cur].num_neighbors; j++) {
                    int nid = tiles[cur].neighbors[j]->id;
                    if (sim_state[nid] == 0) {
                        sim_state[nid] = 1;
                        queue[q_tail++] = nid;
                        progress = true;
                    }
                }
            }
        }
        
        if (progress) continue;
        
        // Tier 2: Subset reduction
        int f_count = 0;
        for(int i=0; i<T; i++) {
            if (sim_state[i] == 1 && tiles[i].adjacent_mines > 0) {
                for(int j=0; j<tiles[i].num_neighbors; j++) {
                    if (sim_state[tiles[i].neighbors[j]->id] == 0) {
                        frontier[f_count++] = i;
                        break;
                    }
                }
            }
        }
        
        for(int a=0; a<f_count && !progress; a++) {
            int idA = frontier[a];
            int neededA = tiles[idA].adjacent_mines;
            int hiddenA[16];
            int hCountA = 0;
            for(int j=0; j<tiles[idA].num_neighbors; j++) {
                int nid = tiles[idA].neighbors[j]->id;
                if (sim_state[nid] == 2) neededA--;
                else if (sim_state[nid] == 0) hiddenA[hCountA++] = nid;
            }
            if (hCountA == 0) continue;
            
            for(int b=0; b<f_count && !progress; b++) {
                if (a == b) continue;
                int idB = frontier[b];
                int neededB = tiles[idB].adjacent_mines;
                int hiddenB[16];
                int hCountB = 0;
                for(int j=0; j<tiles[idB].num_neighbors; j++) {
                    int nid = tiles[idB].neighbors[j]->id;
                    if (sim_state[nid] == 2) neededB--;
                    else if (sim_state[nid] == 0) hiddenB[hCountB++] = nid;
                }
                if (hCountB <= hCountA) continue;
                
                bool is_subset = true;
                for(int ia=0; ia<hCountA; ia++) {
                    bool found = false;
                    for(int ib=0; ib<hCountB; ib++) {
                        if (hiddenA[ia] == hiddenB[ib]) { found = true; break; }
                    }
                    if (!found) { is_subset = false; break; }
                }
                
                if (is_subset) {
                    int diff[16];
                    int diff_count = 0;
                    for(int ib=0; ib<hCountB; ib++) {
                        bool inA = false;
                        for(int ia=0; ia<hCountA; ia++) {
                            if (hiddenB[ib] == hiddenA[ia]) { inA = true; break; }
                        }
                        if (!inA) diff[diff_count++] = hiddenB[ib];
                    }
                    
                    int diff_needed = neededB - neededA;
                    if (diff_count > 0) {
                        if (diff_needed == 0) {
                            for(int d=0; d<diff_count; d++) {
                                sim_state[diff[d]] = 1;
                                queue[q_tail++] = diff[d];
                                progress = true;
                            }
                        } else if (diff_needed == diff_count) {
                            for(int d=0; d<diff_count; d++) {
                                sim_state[diff[d]] = 2;
                                progress = true;
                            }
                        }
                    }
                }
            }
        }
        
        while(q_head < q_tail) {
            int cur = queue[q_head++];
            if (tiles[cur].adjacent_mines == 0) {
                for(int j=0; j<tiles[cur].num_neighbors; j++) {
                    int nid = tiles[cur].neighbors[j]->id;
                    if (sim_state[nid] == 0) {
                        sim_state[nid] = 1;
                        queue[q_tail++] = nid;
                        progress = true;
                    }
                }
            }
        }
    }
    
    int revealed_count = 0;
    for(int i=0; i<T; i++) {
        if (sim_state[i] == 1) revealed_count++;
    }
    
    bool solvable = (revealed_count == T - M);
    free(sim_state);
    free(queue);
    free(frontier);
    return solvable;
}

void place_mines(int safe_id) {
    int* pool = (int*)malloc(sizeof(int) * T);
    int pool_size = 0;
    
    for(int i=0; i<T; i++) {
        bool safe = false;
        if (i == safe_id) safe = true;
        for(int j=0; j<tiles[safe_id].num_neighbors; j++) {
            if (tiles[safe_id].neighbors[j]->id == i) safe = true;
        }
        if (!safe) pool[pool_size++] = i;
    }
    
    int max_attempts = 1000;
    for(int attempt = 0; attempt < max_attempts; attempt++) {
        for(int i=0; i<T; i++) {
            tiles[i].is_mine = false;
            tiles[i].adjacent_mines = 0;
        }
        
        for (int i = pool_size - 1; i > 0; i--) {
            int j = rand() % (i + 1);
            int temp = pool[i];
            pool[i] = pool[j];
            pool[j] = temp;
        }
        
        for(int i=0; i<M && i<pool_size; i++) {
            tiles[pool[i]].is_mine = true;
        }
        
        for(int i=0; i<T; i++) {
            if (tiles[i].is_mine) continue;
            int count = 0;
            for(int j=0; j<tiles[i].num_neighbors; j++) {
                if (tiles[i].neighbors[j]->is_mine) count++;
            }
            tiles[i].adjacent_mines = count;
        }
        
        if (verify_surface_board_solvable(safe_id)) {
            break; // 100% no-guess board found!
        }
    }
    
    free(pool);
}

void reveal_cascade(int id) {
    if (tiles[id].is_revealed || tiles[id].is_flagged) return;
    tiles[id].is_revealed = true;
    
    if (tiles[id].is_mine) {
        game_over = true;
        for(int i=0; i<T; i++) {
            if (tiles[i].is_mine) tiles[i].is_revealed = true;
        }
        return;
    }
    
    if (tiles[id].adjacent_mines == 0) {
        for(int i=0; i<tiles[id].num_neighbors; i++) {
            reveal_cascade(tiles[id].neighbors[i]->id);
        }
    }
}

void check_win() {
    if (game_over) return;
    int safe_unrevealed = 0;
    for(int i=0; i<T; i++) {
        if (!tiles[i].is_mine && !tiles[i].is_revealed) safe_unrevealed++;
    }
    if (safe_unrevealed == 0) game_won = true;
}

// --------------------------------------------------------
// Block Mode (Volumetric Excavation) Logic
// --------------------------------------------------------

void generate_block_grid() {
    H = 3 + rand() % 3; // 3 to 5
    W = 3 + rand() % 3;
    D = 3 + rand() % 3;
    
    total_blocks = H * W * D;
    blocks = (Block*)malloc(sizeof(Block) * total_blocks);
    
    for(int y=0; y<H; y++) {
        for(int x=0; x<W; x++) {
            for(int z=0; z<D; z++) {
                int id = (y * W + x) * D + z;
                Block* b = &blocks[id];
                b->id = id;
                b->x = x; b->y = y; b->z = z;
                b->center = (vec3){ x - (W-1)/2.0f, (H-1)/2.0f - y, z - (D-1)/2.0f };
                b->is_mine = false;
                b->is_revealed = false;
                b->is_flagged = false;
                b->adjacent_mines = 0;
                b->num_neighbors = 0;
                
                for(int dy=-1; dy<=1; dy++) {
                    for(int dx=-1; dx<=1; dx++) {
                        for(int dz=-1; dz<=1; dz++) {
                            if (dx == 0 && dy == 0 && dz == 0) continue;
                            int nx = x + dx, ny = y + dy, nz = z + dz;
                            if (nx >= 0 && nx < W && ny >= 0 && ny < H && nz >= 0 && nz < D) {
                                int nid = (ny * W + nx) * D + nz;
                                b->neighbors[b->num_neighbors++] = nid;
                            }
                        }
                    }
                }
            }
        }
    }
    
    M = (int)(total_blocks * 0.15f);
    if (M < 1) M = 1;
    distanceFromCam = fmaxf(H, fmaxf(W, D)) * 1.5f + 10.0f;
}

bool verify_block_board_solvable(int safe_id) {
    int *sim_state = (int*)calloc(total_blocks, sizeof(int));
    int *queue = (int*)malloc(sizeof(int) * total_blocks);
    int *frontier = (int*)malloc(sizeof(int) * total_blocks);
    int q_head = 0, q_tail = 0;
    
    sim_state[safe_id] = 1;
    queue[q_tail++] = safe_id;
    
    while(q_head < q_tail) {
        int cur = queue[q_head++];
        if (blocks[cur].adjacent_mines == 0) {
            for(int j=0; j<blocks[cur].num_neighbors; j++) {
                int nid = blocks[cur].neighbors[j];
                if (sim_state[nid] == 0) {
                    sim_state[nid] = 1;
                    queue[q_tail++] = nid;
                }
            }
        }
    }
    
    bool progress = true;
    while(progress) {
        progress = false;
        
        // Tier 1: Direct counting
        for(int i=0; i<total_blocks; i++) {
            if (sim_state[i] != 1 || blocks[i].adjacent_mines == 0) continue;
            
            int hidden_count = 0;
            int flagged_count = 0;
            for(int j=0; j<blocks[i].num_neighbors; j++) {
                int nid = blocks[i].neighbors[j];
                if (sim_state[nid] == 0) hidden_count++;
                else if (sim_state[nid] == 2) flagged_count++;
            }
            
            int needed = blocks[i].adjacent_mines - flagged_count;
            if (hidden_count > 0) {
                if (hidden_count == needed) {
                    for(int j=0; j<blocks[i].num_neighbors; j++) {
                        int nid = blocks[i].neighbors[j];
                        if (sim_state[nid] == 0) {
                            sim_state[nid] = 2;
                            progress = true;
                        }
                    }
                } else if (needed == 0) {
                    for(int j=0; j<blocks[i].num_neighbors; j++) {
                        int nid = blocks[i].neighbors[j];
                        if (sim_state[nid] == 0) {
                            sim_state[nid] = 1;
                            queue[q_tail++] = nid;
                            progress = true;
                        }
                    }
                }
            }
        }
        
        while(q_head < q_tail) {
            int cur = queue[q_head++];
            if (blocks[cur].adjacent_mines == 0) {
                for(int j=0; j<blocks[cur].num_neighbors; j++) {
                    int nid = blocks[cur].neighbors[j];
                    if (sim_state[nid] == 0) {
                        sim_state[nid] = 1;
                        queue[q_tail++] = nid;
                        progress = true;
                    }
                }
            }
        }
        
        if (progress) continue;
        
        // Tier 2: Subset reduction
        int f_count = 0;
        for(int i=0; i<total_blocks; i++) {
            if (sim_state[i] == 1 && blocks[i].adjacent_mines > 0) {
                for(int j=0; j<blocks[i].num_neighbors; j++) {
                    if (sim_state[blocks[i].neighbors[j]] == 0) {
                        frontier[f_count++] = i;
                        break;
                    }
                }
            }
        }
        
        for(int a=0; a<f_count && !progress; a++) {
            int idA = frontier[a];
            int neededA = blocks[idA].adjacent_mines;
            int hiddenA[32];
            int hCountA = 0;
            for(int j=0; j<blocks[idA].num_neighbors; j++) {
                int nid = blocks[idA].neighbors[j];
                if (sim_state[nid] == 2) neededA--;
                else if (sim_state[nid] == 0) hiddenA[hCountA++] = nid;
            }
            if (hCountA == 0) continue;
            
            for(int b=0; b<f_count && !progress; b++) {
                if (a == b) continue;
                int idB = frontier[b];
                int neededB = blocks[idB].adjacent_mines;
                int hiddenB[32];
                int hCountB = 0;
                for(int j=0; j<blocks[idB].num_neighbors; j++) {
                    int nid = blocks[idB].neighbors[j];
                    if (sim_state[nid] == 2) neededB--;
                    else if (sim_state[nid] == 0) hiddenB[hCountB++] = nid;
                }
                if (hCountB <= hCountA) continue;
                
                bool is_subset = true;
                for(int ia=0; ia<hCountA; ia++) {
                    bool found = false;
                    for(int ib=0; ib<hCountB; ib++) {
                        if (hiddenA[ia] == hiddenB[ib]) { found = true; break; }
                    }
                    if (!found) { is_subset = false; break; }
                }
                
                if (is_subset) {
                    int diff[32];
                    int diff_count = 0;
                    for(int ib=0; ib<hCountB; ib++) {
                        bool inA = false;
                        for(int ia=0; ia<hCountA; ia++) {
                            if (hiddenB[ib] == hiddenA[ia]) { inA = true; break; }
                        }
                        if (!inA) diff[diff_count++] = hiddenB[ib];
                    }
                    
                    int diff_needed = neededB - neededA;
                    if (diff_count > 0) {
                        if (diff_needed == 0) {
                            for(int d=0; d<diff_count; d++) {
                                sim_state[diff[d]] = 1;
                                queue[q_tail++] = diff[d];
                                progress = true;
                            }
                        } else if (diff_needed == diff_count) {
                            for(int d=0; d<diff_count; d++) {
                                sim_state[diff[d]] = 2;
                                progress = true;
                            }
                        }
                    }
                }
            }
        }
        
        while(q_head < q_tail) {
            int cur = queue[q_head++];
            if (blocks[cur].adjacent_mines == 0) {
                for(int j=0; j<blocks[cur].num_neighbors; j++) {
                    int nid = blocks[cur].neighbors[j];
                    if (sim_state[nid] == 0) {
                        sim_state[nid] = 1;
                        queue[q_tail++] = nid;
                        progress = true;
                    }
                }
            }
        }
    }
    
    int revealed_count = 0;
    for(int i=0; i<total_blocks; i++) {
        if (sim_state[i] == 1) revealed_count++;
    }
    
    bool solvable = (revealed_count == total_blocks - M);
    free(sim_state);
    free(queue);
    free(frontier);
    return solvable;
}

void place_block_mines(int safe_id) {
    int* pool = (int*)malloc(sizeof(int) * total_blocks);
    int pool_size = 0;
    
    for(int i=0; i<total_blocks; i++) {
        bool safe = false;
        if (i == safe_id) safe = true;
        for(int j=0; j<blocks[safe_id].num_neighbors; j++) {
            if (blocks[safe_id].neighbors[j] == i) safe = true;
        }
        if (!safe) pool[pool_size++] = i;
    }
    
    int max_attempts = 1000;
    for(int attempt = 0; attempt < max_attempts; attempt++) {
        for(int i=0; i<total_blocks; i++) {
            blocks[i].is_mine = false;
            blocks[i].adjacent_mines = 0;
        }
        
        for (int i = pool_size - 1; i > 0; i--) {
            int j = rand() % (i + 1);
            int temp = pool[i];
            pool[i] = pool[j];
            pool[j] = temp;
        }
        
        for(int i=0; i<M && i<pool_size; i++) {
            blocks[pool[i]].is_mine = true;
        }
        
        for(int i=0; i<total_blocks; i++) {
            if (blocks[i].is_mine) continue;
            int count = 0;
            for(int j=0; j<blocks[i].num_neighbors; j++) {
                if (blocks[blocks[i].neighbors[j]].is_mine) count++;
            }
            blocks[i].adjacent_mines = count;
        }
        
        if (verify_block_board_solvable(safe_id)) {
            break; // 100% no-guess board found!
        }
    }
    
    free(pool);
}

void reveal_block_cascade(int id) {
    if (blocks[id].is_revealed || blocks[id].is_flagged) return;
    blocks[id].is_revealed = true;
    
    if (blocks[id].is_mine) {
        game_over = true;
        for(int i=0; i<total_blocks; i++) {
            if (blocks[i].is_mine) blocks[i].is_revealed = true;
        }
        return;
    }
    
    if (blocks[id].adjacent_mines == 0) {
        for(int i=0; i<blocks[id].num_neighbors; i++) {
            reveal_block_cascade(blocks[id].neighbors[i]);
        }
    }
}

void check_block_win() {
    if (game_over) return;
    int safe_unrevealed = 0;
    for(int i=0; i<total_blocks; i++) {
        if (!blocks[i].is_mine && !blocks[i].is_revealed) safe_unrevealed++;
    }
    if (safe_unrevealed == 0) game_won = true;
}

void init_game() {
    if (tiles) { free(tiles); tiles = NULL; }
    if (blocks) { free(blocks); blocks = NULL; }
    first_click = true;
    game_over = false;
    game_won = false;
    if (current_mode == MODE_SURFACE) {
        generate_cuboid();
    } else {
        generate_block_grid();
    }
}

// --------------------------------------------------------
// Rendering Helpers & Overlays
// --------------------------------------------------------

void draw_glyph_3x5(int cx, int cy, char sym, int col, int target_fid, int scale_x) {
    int bits = get_sweeper_font_bits(sym);
    int start_x = cx - (scale_x == 2 ? 3 : 1);
    int start_y = cy - 2;
    for(int dy = 0; dy < 5; dy++) {
        for(int dx = 0; dx < 3; dx++) {
            int bit_idx = 14 - (dy * 3 + dx);
            if ((bits >> bit_idx) & 1) {
                for(int sx = 0; sx < scale_x; sx++) {
                    int px = start_x + dx * scale_x + sx;
                    int py = start_y + dy;
                    if (px >= 0 && px < width && py >= 0 && py < height) {
                        if (face_buffer[py * width + px] == target_fid) {
                            char_buffer[py * width + px] = sym;
                            color_buffer[py * width + px] = col;
                        }
                    }
                }
            }
        }
    }
}

void draw_glyph_3x3(int cx, int cy, char sym, int col, int target_fid) {
    int bits = get_sweeper_font_bits_3x3(sym);
    int start_x = cx - 1;
    int start_y = cy - 1;
    for(int dy = 0; dy < 3; dy++) {
        for(int dx = 0; dx < 3; dx++) {
            int bit_idx = 8 - (dy * 3 + dx);
            if ((bits >> bit_idx) & 1) {
                int px = start_x + dx;
                int py = start_y + dy;
                if (px >= 0 && px < width && py >= 0 && py < height) {
                    if (face_buffer[py * width + px] == target_fid) {
                        char_buffer[py * width + px] = sym;
                        color_buffer[py * width + px] = col;
                    }
                }
            }
        }
    }
}

void draw_glyph(int cx, int cy, int w, int h, char sym, int col, int target_fid) {
    if (w >= 8 && h >= 5) {
        draw_glyph_3x5(cx, cy, sym, col, target_fid, 2);
    } else if (w >= 4 && h >= 5) {
        draw_glyph_3x5(cx, cy, sym, col, target_fid, 1);
    } else if (w >= 3 && h >= 3) {
        draw_glyph_3x3(cx, cy, sym, col, target_fid);
    } else {
        if (cx >= 0 && cx < width && cy >= 0 && cy < height) {
            if (face_buffer[cy * width + cx] == target_fid) {
                char_buffer[cy * width + cx] = sym;
                color_buffer[cy * width + cx] = col;
            }
        }
    }
}

void render_surface_geometry() {
    for(int i=0; i<T; i++) {
        char ch = '#';
        int color = 0;
        if (tiles[i].is_revealed) {
            if (tiles[i].is_mine) { ch = '*'; color = 91; }
            else { ch = '#'; color = 90; } // Revealed safe tiles are solid dark gray stone
        } else {
            ch = '#'; color = 0;
        }
        rasterize_triangle(tiles[i].v[0], tiles[i].v[1], tiles[i].v[2], color, ch, tiles[i].id, tiles[i].id);
        rasterize_triangle(tiles[i].v[2], tiles[i].v[3], tiles[i].v[0], color, ch, tiles[i].id, tiles[i].id);
    }
}

void apply_tile_inner_padding() {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = y * width + x;
            int fid = face_buffer[idx];
            if (fid < 0) continue;
            
            bool is_border = false;
            // Check right (+X) neighbor: only pad if column to the left is also this fid (tile is at least 2 cols wide)
            if (x + 1 < width) {
                int nfid = face_buffer[y * width + (x + 1)];
                if (nfid >= 0 && nfid != fid) {
                    if (x > 0 && face_buffer[y * width + (x - 1)] == fid) {
                        is_border = true;
                    }
                }
            }
            // Check down (+Y) neighbor: only pad if row above is also this fid (tile is at least 2 rows tall)
            if (!is_border && y + 1 < height) {
                int nfid = face_buffer[(y + 1) * width + x];
                if (nfid >= 0 && nfid != fid) {
                    if (y > 0 && face_buffer[(y - 1) * width + x] == fid) {
                        is_border = true;
                    }
                }
            }
            
            if (is_border) {
                char_buffer[idx] = ' ';
                color_buffer[idx] = 0;
            }
        }
    }
}

void render_surface_overlays(int hovered_id) {
    for(int i=0; i<T; i++) {
        if (!tiles[i].is_flagged && (!tiles[i].is_revealed || tiles[i].adjacent_mines == 0)) {
            if (!(tiles[i].is_revealed && tiles[i].is_mine)) continue;
        }
        
        vec3 cam_norm;
        apply_camera(tiles[i].normal, &cam_norm);
        if (cam_norm.z >= 0.0f) continue;
        
        float min_px = 1e9f, max_px = -1e9f, min_py = 1e9f, max_py = -1e9f;
        bool all_valid = true;
        for(int v=0; v<4; v++) {
            vec3 pv;
            apply_camera(tiles[i].v[v], &pv);
            pv.z += distanceFromCam;
            if (pv.z <= 0.1f) { all_valid = false; break; }
            float oozv = 1.0f / pv.z;
            float vx = width * 0.5f + (pv.x * oozv) * K1 * 2.0f;
            float vy = height * 0.5f + (pv.y * oozv) * K1;
            if (vx < min_px) min_px = vx;
            if (vx > max_px) max_px = vx;
            if (vy < min_py) min_py = vy;
            if (vy > max_py) max_py = vy;
        }
        if (!all_valid) continue;
        
        int cx = (int)((min_px + max_px) * 0.5f);
        int cy = (int)((min_py + max_py) * 0.5f);
        if (cx < 0 || cx >= width || cy < 0 || cy >= height) continue;
        
        int test_id = id_buffer[cy * width + cx];
        if (test_id != tiles[i].id) {
            if (cy > 0 && id_buffer[(cy-1)*width + cx] == tiles[i].id) test_id = tiles[i].id;
            else if (cy+1 < height && id_buffer[(cy+1)*width + cx] == tiles[i].id) test_id = tiles[i].id;
            else if (cx > 0 && id_buffer[cy*width + cx - 1] == tiles[i].id) test_id = tiles[i].id;
            else if (cx+1 < width && id_buffer[cy*width + cx + 1] == tiles[i].id) test_id = tiles[i].id;
        }
        if (test_id != tiles[i].id) continue;
        
        int tile_w = (int)(max_px - min_px);
        int tile_h = (int)(max_py - min_py);
        
        char sym = ' ';
        int sym_col = 0;
        if (tiles[i].is_revealed) {
            if (tiles[i].is_mine) { sym = '*'; sym_col = 91; }
            else {
                sym = '0' + tiles[i].adjacent_mines;
                int cols[] = {0, 94, 32, 91, 35, 33, 36, 90, 37};
                sym_col = cols[tiles[i].adjacent_mines];
            }
        } else if (tiles[i].is_flagged) {
            sym = 'F';
            sym_col = (i == hovered_id) ? 193 : 93;
        }
        
        draw_glyph(cx, cy, tile_w, tile_h, sym, sym_col, tiles[i].id);
    }
}

void get_block_face_vertices(vec3 c, float s, int f, vec3 v[4]) {
    if (f == 0) { // Front (+Z)
        v[0] = (vec3){c.x - s, c.y + s, c.z + s};
        v[1] = (vec3){c.x + s, c.y + s, c.z + s};
        v[2] = (vec3){c.x + s, c.y - s, c.z + s};
        v[3] = (vec3){c.x - s, c.y - s, c.z + s};
    } else if (f == 1) { // Back (-Z)
        v[0] = (vec3){c.x + s, c.y + s, c.z - s};
        v[1] = (vec3){c.x - s, c.y + s, c.z - s};
        v[2] = (vec3){c.x - s, c.y - s, c.z - s};
        v[3] = (vec3){c.x + s, c.y - s, c.z - s};
    } else if (f == 2) { // Top (+Y)
        v[0] = (vec3){c.x - s, c.y + s, c.z - s};
        v[1] = (vec3){c.x + s, c.y + s, c.z - s};
        v[2] = (vec3){c.x + s, c.y + s, c.z + s};
        v[3] = (vec3){c.x - s, c.y + s, c.z + s};
    } else if (f == 3) { // Bottom (-Y)
        v[0] = (vec3){c.x - s, c.y - s, c.z + s};
        v[1] = (vec3){c.x + s, c.y - s, c.z + s};
        v[2] = (vec3){c.x + s, c.y - s, c.z - s};
        v[3] = (vec3){c.x - s, c.y - s, c.z - s};
    } else if (f == 4) { // Left (-X)
        v[0] = (vec3){c.x - s, c.y + s, c.z - s};
        v[1] = (vec3){c.x - s, c.y + s, c.z + s};
        v[2] = (vec3){c.x - s, c.y - s, c.z + s};
        v[3] = (vec3){c.x - s, c.y - s, c.z - s};
    } else { // Right (+X)
        v[0] = (vec3){c.x + s, c.y + s, c.z + s};
        v[1] = (vec3){c.x + s, c.y + s, c.z - s};
        v[2] = (vec3){c.x + s, c.y - s, c.z - s};
        v[3] = (vec3){c.x + s, c.y - s, c.z + s};
    }
}

bool is_block_face_exposed(Block *b, int f) {
    if (b->is_revealed) return true; // Contracted central node has all faces exposed to open air
    
    const int f_dirs[6][3] = {
        {0, 0, 1},   // 0: Front (+Z)
        {0, 0, -1},  // 1: Back (-Z)
        {0, -1, 0},  // 2: Top (+Y) -> dy = -1 since center.y = (H-1)/2.0f - y
        {0, 1, 0},   // 3: Bottom (-Y) -> dy = +1
        {-1, 0, 0},  // 4: Left (-X)
        {1, 0, 0}    // 5: Right (+X)
    };
    int nx = b->x + f_dirs[f][0];
    int ny = b->y + f_dirs[f][1];
    int nz = b->z + f_dirs[f][2];
    if (nx < 0 || nx >= W || ny < 0 || ny >= H || nz < 0 || nz >= D) {
        return true; // outside air
    }
    int nid = (ny * W + nx) * D + nz;
    // Neighbor is revealed: excavated air cavern or contracted node leaves this face exposed
    if (blocks[nid].is_revealed && !blocks[nid].is_mine) {
        return true;
    }
    return false;
}

void render_block_geometry() {
    for(int i=0; i<total_blocks; i++) {
        Block* b = &blocks[i];
        if (b->is_revealed && b->adjacent_mines == 0 && !b->is_mine) continue; // Excavated air!
        
        // Contracted to 0.3 total size (s = 0.15f) for revealed numbered blocks, full size (s = 0.5f) for unrevealed
        float s = (b->is_revealed && !b->is_mine) ? 0.25f : 0.5f;
        vec3 c = b->center;
        for(int f=0; f<6; f++) {
            if (!is_block_face_exposed(b, f)) continue;
            
            vec3 v[4];
            get_block_face_vertices(c, s, f, v);
            
            char ch = '#';
            int color = 0;
            if (b->is_revealed) {
                if (b->is_mine) { ch = '*'; color = 91; }
                else { ch = '#'; color = 90; } // Contracted clue node: dark gray pedestal
            } else {
                ch = '#'; color = 0;
            }
            
            int face_id = b->id * 6 + f;
            rasterize_triangle(v[0], v[1], v[2], color, ch, b->id, face_id);
            rasterize_triangle(v[2], v[3], v[0], color, ch, b->id, face_id);
        }
    }
}

void render_block_overlays(int hovered_id) {
    const vec3 face_normals[6] = {
        {0, 0, 1},   // 0: Front (+Z)
        {0, 0, -1},  // 1: Back (-Z)
        {0, 1, 0},   // 2: Top (+Y)
        {0, -1, 0},  // 3: Bottom (-Y)
        {-1, 0, 0},  // 4: Left (-X)
        {1, 0, 0}    // 5: Right (+X)
    };
    
    for(int i=0; i<total_blocks; i++) {
        Block* b = &blocks[i];
        if (!b->is_flagged && (!b->is_revealed || b->adjacent_mines == 0)) {
            if (!(b->is_revealed && b->is_mine)) continue;
        }
        
        float s = (b->is_revealed && !b->is_mine) ? 0.25f : 0.5f;
        vec3 c = b->center;
        for(int f=0; f<6; f++) {
            if (!is_block_face_exposed(b, f)) continue;
            
            vec3 cam_norm;
            apply_camera(face_normals[f], &cam_norm);
            if (cam_norm.z >= 0.0f) continue; // Cull back-facing faces
            
            vec3 v[4];
            get_block_face_vertices(c, s, f, v);
            
            float min_px = 1e9f, max_px = -1e9f, min_py = 1e9f, max_py = -1e9f;
            bool all_valid = true;
            for(int vi=0; vi<4; vi++) {
                vec3 pv;
                apply_camera(v[vi], &pv);
                pv.z += distanceFromCam;
                if (pv.z <= 0.1f) { all_valid = false; break; }
                float oozv = 1.0f / pv.z;
                float vx = width * 0.5f + (pv.x * oozv) * K1 * 2.0f;
                float vy = height * 0.5f + (pv.y * oozv) * K1;
                if (vx < min_px) min_px = vx;
                if (vx > max_px) max_px = vx;
                if (vy < min_py) min_py = vy;
                if (vy > max_py) max_py = vy;
            }
            if (!all_valid) continue;
            
            int cx = (int)((min_px + max_px) * 0.5f);
            int cy = (int)((min_py + max_py) * 0.5f);
            if (cx < 0 || cx >= width || cy < 0 || cy >= height) continue;
            
            int face_id = b->id * 6 + f;
            int test_fid = face_buffer[cy * width + cx];
            if (test_fid != face_id) {
                if (cy > 0 && face_buffer[(cy-1)*width + cx] == face_id) { test_fid = face_id; cy--; }
                else if (cy+1 < height && face_buffer[(cy+1)*width + cx] == face_id) { test_fid = face_id; cy++; }
                else if (cx > 0 && face_buffer[cy*width + cx - 1] == face_id) { test_fid = face_id; cx--; }
                else if (cx+1 < width && face_buffer[cy*width + cx + 1] == face_id) { test_fid = face_id; cx++; }
            }
            if (test_fid != face_id) continue;
            
            int face_w = (int)(max_px - min_px);
            int face_h = (int)(max_py - min_py);
            
            if (b->is_flagged) {
                char sym = 'F';
                int sym_col = (b->id == hovered_id) ? 193 : 93;
                draw_glyph(cx, cy, face_w, face_h, sym, sym_col, face_id);
            } else if (b->is_revealed && b->is_mine) {
                char sym = '*';
                int sym_col = 91;
                draw_glyph(cx, cy, face_w, face_h, sym, sym_col, face_id);
            } else if (b->is_revealed && b->adjacent_mines > 0) {
                int val = b->adjacent_mines;
                int cols[] = {0, 94, 32, 91, 35, 33, 36, 91, 37, 95, 93, 96, 92, 94, 35, 33, 36, 91, 37, 95, 93, 96, 92, 94, 35, 33, 36};
                int sym_col = cols[(val <= 26) ? val : 26];
                
                if (val < 10) {
                    char sym = '0' + val;
                    draw_glyph(cx, cy, face_w, face_h, sym, sym_col, face_id);
                } else {
                    char d1 = '0' + (val / 10);
                    char d2 = '0' + (val % 10);
                    if (face_w >= 10 && face_h >= 5) {
                        draw_glyph_3x5(cx - 3, cy, d1, sym_col, face_id, 1);
                        draw_glyph_3x5(cx + 2, cy, d2, sym_col, face_id, 1);
                    } else if (face_w >= 6 && face_h >= 3) {
                        draw_glyph_3x3(cx - 2, cy, d1, sym_col, face_id);
                        draw_glyph_3x3(cx + 2, cy, d2, sym_col, face_id);
                    } else {
                        if (cx - 1 >= 0 && face_buffer[cy * width + cx - 1] == face_id) {
                            char_buffer[cy * width + cx - 1] = d1;
                            color_buffer[cy * width + cx - 1] = sym_col;
                        }
                        if (cx < width && face_buffer[cy * width + cx] == face_id) {
                            char_buffer[cy * width + cx] = d2;
                            color_buffer[cy * width + cx] = sym_col;
                        }
                    }
                }
            }
        }
    }
}

// --------------------------------------------------------
// Main Loop
// --------------------------------------------------------

int main() {
    srand((unsigned int)time(NULL));
    
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
    printf("\x1b[?25l"); // hide cursor
    
    init_game();
    bool running = true;
    
    while(running) {
        int w, h;
        getTerminalSize(&w, &h);
        if (w != width || h != height) {
            width = w; height = h;
            zBuffer = (float*)realloc(zBuffer, sizeof(float) * width * height);
            char_buffer = (char*)realloc(char_buffer, sizeof(char) * width * height);
            color_buffer = (int*)realloc(color_buffer, sizeof(int) * width * height);
            id_buffer = (int*)realloc(id_buffer, sizeof(int) * width * height);
            face_buffer = (int*)realloc(face_buffer, sizeof(int) * width * height);
            renderBuffer = (char*)realloc(renderBuffer, (width + margin_x + 16) * height * 24 + height + 2048);
            K1 = height * 0.9f;
        }
        
        for (int i = 0; i < width * height; i++) {
            zBuffer[i] = 10000.0f;
            char_buffer[i] = ' ';
            color_buffer[i] = 0;
            id_buffer[i] = -1;
            face_buffer[i] = -1;
        }
        
        bool do_reveal = false;
        bool do_flag = false;
        
#ifdef _WIN32
        INPUT_RECORD ir[32];
        DWORD read;
        GetNumberOfConsoleInputEvents(hIn, &read);
        if (read > 0) {
            ReadConsoleInput(hIn, ir, 32, &read);
            for(DWORD i = 0; i < read; i++) {
                if (ir[i].EventType == KEY_EVENT && ir[i].Event.KeyEvent.bKeyDown) {
                    char c = ir[i].Event.KeyEvent.uChar.AsciiChar;
                    if (c == 'q' || c == 'Q' || ir[i].Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE) running = false;
                    if (c == 'w' || c == 'W') { camA -= 0.075f; if (camA < -1.5533f) camA = -1.5533f; }
                    if (c == 's' || c == 'S') { camA += 0.075f; if (camA > 1.5533f) camA = 1.5533f; }
                    if (c == 'a' || c == 'A') camB += 0.075f;
                    if (c == 'd' || c == 'D') camB -= 0.075f;
                    if (c == 'm' || c == 'M') {
                        current_mode = (current_mode == MODE_SURFACE) ? MODE_BLOCK : MODE_SURFACE;
                        init_game();
                    }
                    if (c == 'r' || c == 'R') {
                        init_game();
                    }
                }
                if (ir[i].EventType == MOUSE_EVENT) {
                    MOUSE_EVENT_RECORD mer = ir[i].Event.MouseEvent;
                    int cur_x = mer.dwMousePosition.X - margin_x;
                    int cur_y = mer.dwMousePosition.Y;
                    mouse_x = cur_x;
                    mouse_y = cur_y;
                    
                    if (mer.dwEventFlags & MOUSE_WHEELED) {
                        short wheel_delta = (short)HIWORD(mer.dwButtonState);
                        if (wheel_delta > 0) zoom_camera(-1.5f);
                        else if (wheel_delta < 0) zoom_camera(1.5f);
                    }
                    
                    bool left_down = (mer.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED) != 0;
                    bool right_down = (mer.dwButtonState & RIGHTMOST_BUTTON_PRESSED) != 0;
                    bool mid_down = (mer.dwButtonState & FROM_LEFT_2ND_BUTTON_PRESSED) != 0;
                    
                    // Left-click drag to orbit (inverted horizontal yaw)
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
                        // Left button released
                        if (!is_mouse_dragging) {
                            long long now = GetTickCount64();
                            if (now - last_left_click_time < 500) {
                                do_reveal = true;
                                last_left_click_time = 0;
                            } else {
                                last_left_click_time = now;
                            }
                        } else {
                            last_left_click_time = 0;
                            is_mouse_dragging = false;
                        }
                    }
                    
                    // Middle-click drag to orbit (inverted horizontal yaw)
                    if (mid_down) {
                        if (!mid_down_prev) {
                            last_mid_x = cur_x;
                            last_mid_y = cur_y;
                        } else {
                            int dx = cur_x - last_mid_x;
                            int dy = cur_y - last_mid_y;
                            if (dx != 0 || dy != 0) {
                                camB -= (float)dx * 0.0175f;
                                camA += (float)dy * 0.0325f;
                                if (camA > 1.5533f) camA = 1.5533f;
                                if (camA < -1.5533f) camA = -1.5533f;
                                last_mid_x = cur_x;
                                last_mid_y = cur_y;
                            }
                        }
                    }
                    
                    if (right_down && !right_down_prev) {
                        do_flag = true;
                    }
                    
                    left_down_prev = left_down;
                    right_down_prev = right_down;
                    mid_down_prev = mid_down;
                }
            }
        }
#else
        int ch;
        while ((ch = getchar()) != EOF) {
            if (ch == 'q' || ch == 'Q') running = false;
            if (ch == 'w' || ch == 'W') { camA -= 0.075f; if (camA < -1.5533f) camA = -1.5533f; }
            if (ch == 's' || ch == 'S') { camA += 0.075f; if (camA > 1.5533f) camA = 1.5533f; }
            if (ch == 'a' || ch == 'A') camB += 0.075f;
            if (ch == 'd' || ch == 'D') camB -= 0.075f;
            if (ch == 'm' || ch == 'M') {
                current_mode = (current_mode == MODE_SURFACE) ? MODE_BLOCK : MODE_SURFACE;
                init_game();
            }
            if (ch == 'r' || ch == 'R') {
                init_game();
            }
            if (ch == '\033') {
                if (getchar() == '[') {
                    if (getchar() == '<') {
                        int b = 0, mx = 0, my = 0;
                        char m_state;
                        scanf("%d;%d;%d%c", &b, &mx, &my, &m_state);
                        int cur_x = mx - 1 - margin_x;
                        int cur_y = my - 1;
                        mouse_x = cur_x;
                        mouse_y = cur_y;
                        if (b == 64) {
                            zoom_camera(-1.5f);
                        } else if (b == 65) {
                            zoom_camera(1.5f);
                        } else if (m_state == 'M') { // Press or Move
                            if (b == 0) { // Left press
                                drag_start_x = cur_x;
                                drag_start_y = cur_y;
                                last_drag_x = cur_x;
                                last_drag_y = cur_y;
                                is_mouse_dragging = false;
                                left_down_prev = true;
                            } else if (b == 32) { // Left drag (motion with button 0 down)
                                int dx = cur_x - last_drag_x;
                                int dy = cur_y - last_drag_y;
                                if (abs(cur_x - drag_start_x) > 1 || abs(cur_y - drag_start_y) > 1) {
                                    is_mouse_dragging = true;
                                }
                                if (is_mouse_dragging && (dx != 0 || dy != 0)) {
                                    camB -= (float)dx * 0.0175f;
                                    camA += (float)dy * 0.0325f;
                                    if (camA > 1.5533f) camA = 1.5533f;
                                    if (camA < -1.5533f) camA = -1.5533f;
                                    last_drag_x = cur_x;
                                    last_drag_y = cur_y;
                                }
                            } else if (b == 2) { // Right click
                                do_flag = true;
                            }
                        } else if (m_state == 'm') { // Release
                            if (b == 0) {
                                if (!is_mouse_dragging) {
                                    long long now = get_time_ms();
                                    if (now - last_left_click_time < 500) {
                                        do_reveal = true;
                                        last_left_click_time = 0;
                                    } else {
                                        last_left_click_time = now;
                                    }
                                } else {
                                    last_left_click_time = 0;
                                    is_mouse_dragging = false;
                                }
                                left_down_prev = false;
                            }
                        }
                    }
                }
            }
        }
#endif

        if (current_mode == MODE_SURFACE) {
            render_surface_geometry();
        } else {
            render_block_geometry();
        }
        
        int hovered_id = -1;
        if (mouse_x >= 0 && mouse_x < width && mouse_y >= 0 && mouse_y < height) {
            hovered_id = id_buffer[mouse_y * width + mouse_x];
            // Snap to 1-pixel neighbor if hovering directly on the groove
            if (hovered_id == -1) {
                for (int dy = -1; dy <= 1 && hovered_id == -1; dy++) {
                    for (int dx = -1; dx <= 1; dx++) {
                        int ny = mouse_y + dy, nx = mouse_x + dx;
                        if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                            if (id_buffer[ny * width + nx] != -1) {
                                hovered_id = id_buffer[ny * width + nx];
                                break;
                            }
                        }
                    }
                }
            }
        }
        
        if (hovered_id != -1 && !game_over && !game_won) {
            if (current_mode == MODE_SURFACE) {
                if (!tiles[hovered_id].is_revealed && !is_mouse_dragging) {
                    for(int j=0; j<width*height; j++) {
                        if (id_buffer[j] == hovered_id) color_buffer[j] = 100;
                    }
                }
                if (do_reveal && !tiles[hovered_id].is_flagged) {
                    if (first_click) {
                        place_mines(hovered_id);
                        first_click = false;
                    }
                    reveal_cascade(hovered_id);
                    check_win();
                }
                if (do_flag && !tiles[hovered_id].is_revealed) {
                    tiles[hovered_id].is_flagged = !tiles[hovered_id].is_flagged;
                }
            } else {
                if (!blocks[hovered_id].is_revealed && !is_mouse_dragging) {
                    for(int j=0; j<width*height; j++) {
                        if (id_buffer[j] == hovered_id) color_buffer[j] = 100;
                    }
                }
                if (do_reveal && !blocks[hovered_id].is_flagged) {
                    if (first_click) {
                        place_block_mines(hovered_id);
                        first_click = false;
                    }
                    reveal_block_cascade(hovered_id);
                    check_block_win();
                }
                if (do_flag && !blocks[hovered_id].is_revealed) {
                    blocks[hovered_id].is_flagged = !blocks[hovered_id].is_flagged;
                }
            }
        }
        
        apply_tile_inner_padding();
        
        if (current_mode == MODE_SURFACE) {
            render_surface_overlays(is_mouse_dragging ? -1 : hovered_id);
        } else {
            render_block_overlays(is_mouse_dragging ? -1 : hovered_id);
        }
        
        int ptr = 0;
        int current_color = 0;
        
        for (int y = 0; y < height; y++) {
            for (int mx = 0; mx < margin_x; mx++) {
                renderBuffer[ptr++] = ' ';
            }
            for (int x = 0; x < width; x++) {
                int idx = y * width + x;
                char c = char_buffer[idx];
                int col = color_buffer[idx];
                
                if (col != current_color) {
                    if (col == 0) {
                        ptr += sprintf(renderBuffer + ptr, "\x1b[0m");
                    } else if (col == 100) {
                        ptr += sprintf(renderBuffer + ptr, "\x1b[0;100m");
                    } else if (col == 193) {
                        ptr += sprintf(renderBuffer + ptr, "\x1b[0;100;93;1m");
                    } else {
                        ptr += sprintf(renderBuffer + ptr, "\x1b[0;%dm", col);
                    }
                    current_color = col;
                }
                renderBuffer[ptr++] = c;
            }
            if (current_color != 0) {
                ptr += sprintf(renderBuffer + ptr, "\x1b[0m");
                current_color = 0;
            }
            renderBuffer[ptr++] = '\n';
        }
        
        char hud[256];
        if (current_mode == MODE_BLOCK) {
            sprintf(hud, "\x1b[H\x1b[1;37mCube Sweeper [BLOCK MODE - NO-GUESS] | H:%d W:%d D:%d | Mines:%d | Drag:Orbit | WASD:Rotate | Scroll:Zoom | DblClick:Dig | R-Click:Flag | M:Mode | R:Restart\x1b[0m", H, W, D, M);
        } else {
            sprintf(hud, "\x1b[H\x1b[1;37mCube Sweeper [SURFACE MODE - NO-GUESS] | H:%d W:%d D:%d | Mines:%d | Drag:Orbit | WASD:Rotate | Scroll:Zoom | DblClick:Reveal | R-Click:Flag | M:Mode | R:Restart\x1b[0m", H, W, D, M);
        }
        
        printf("\x1b[H"); 
        fwrite(renderBuffer, 1, ptr, stdout);
        printf("%s\n", hud);
        
        if (game_over) printf("\x1b[1;31mGAME OVER! You hit a mine. Press 'R' to restart, 'M' to switch mode.\x1b[0m\n");
        if (game_won) {
            if (current_mode == MODE_BLOCK)
                printf("\x1b[1;32mYOU WIN! All safe blocks excavated! Press 'R' to play again, 'M' to switch mode.\x1b[0m\n");
            else
                printf("\x1b[1;32mYOU WIN! All safe tiles cleared! Press 'R' to play again, 'M' to switch mode.\x1b[0m\n");
        }
        
#ifdef _WIN32
        Sleep(16);
#else
        usleep(16000);
#endif
    }
    
    if (tiles) { free(tiles); tiles = NULL; }
    if (blocks) { free(blocks); blocks = NULL; }
    if (face_buffer) { free(face_buffer); face_buffer = NULL; }
    
#ifdef _WIN32
    SetConsoleMode(hIn, origInMode);
#else
    printf("\033[?1003l\033[?1015l\033[?1006l");
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);
#endif
    
    printf("\x1b[?25h"); 
    return 0;
}
