# Gyro 🌀

Real-time 3D geometric visualization and ASCII rendering in pure C.

`Gyro` is a lightweight, zero-dependency terminal graphics playground. It uses custom z-buffering, 3D rotational projection mathematics, and surface normal illumination to render interactive, rotating objects directly inside your terminal window.

---

## 🎬 Demos

### ☀️ Solar System & Planetary Orbits (`src/solar_spinning.c`)
Real-time celestial simulation with a glowing Sun, orbiting planets (Earth, Mars, Jupiter, Saturn with 3D tilted rings, and Moon), dynamic radial point-lighting (day/night shading), and planet camera targeting.

![Solar System Demo](gifs/solar_demo.gif)

---

### 🤖 3D Moving Body & Third-Person Camera (`src/body_moving.c`)
Hierarchically animated robot with smooth walking limb motions, a spatial reference floor grid, full third-person camera orbiting, and WASD movement navigation.

![Moving Body Demo](gifs/body_demo.gif)

---

### 🧬 DNA Double Helix (Interactive) (`src/dna_spinning.c`)
Parametric double helix visualization with real-time interactive camera controls and base-pair rungs.

![DNA Double Helix Demo](gifs/dna_demo.gif)

---

### 🍩 3D Illuminated Donut (`src/donut_spinning.c`)
Real-time surface normal illumination with 12-level ASCII shading.

![3D Donut Demo](gifs/donut_demo.gif)

---

### 🧊 Triple Rotating Cubes (`src/cube_spinning.c`)
Multi-cube projection with independent spatial offsets, depth sorting, and real-time 3D rotation controls.

![3D Cube Demo](gifs/cube_demo.gif)

---

### 🎲 Interactive 3D Rubik's Cube (`src/rubiks_cube.c`)
Fully functional 3×3×3 Rubik's cube rendered with solid colored faces using 2D scanline rasterization. Playable with standard notation inputs and features a live minimap HUD.

---

### 🕳️ Schwarzschild Black Hole (`src/black_hole.c`)
Relativistic raymarching engine simulating a black hole and accretion disk using 4th-order Runge-Kutta integration, gravitational lensing, and TrueColor Doppler beaming.

---

### 💣 3D Cube Sweeper (100% No-Guess Minesweeper) (`src/cube_sweeper.c`)
Fully interactive 3D Minesweeper featuring Surface Mode and true 3D Volumetric Excavation Mode. Powered by an embedded 2-tier deductive constraint solver guaranteeing 100% deterministic, no-guess boards, mouse-drag turntable camera orbiting, and Voxel Contraction to view and click into the internal core.

---

### 🦆 3D Raymarched Duck Hunt (`src/duck_hunt.c`)
Real-time first-person arcade hunting game rendered with Signed Distance Fields (SDF) and volumetric raymarching. Features procedural undulating grass terrain, smooth-blended (`smin`) flapping ducks, mouse crosshair aiming, shotgun raycasts, ammo tracking, and reloading.

---

## Features

- **Solar System & Planetary Orbits** (`src/solar_spinning.c`): Multi-body celestial system with central radial point lighting (day/night hemisphere shading), hierarchical moon orbits, Saturn's ring disc, time speed scaling, and planet camera focus switching.
- **3D Moving Body & Camera** (`src/body_moving.c`): A hierarchically animated robot with smooth limb walking animations, set in a true 3D space with a third-person camera. Control movement with `WASD` and orbit the camera with `IJKL`.
- **DNA Double Helix** (`src/dna_spinning.c`): Parametric double helix visualization with real-time interactive camera controls.
- **3D Torus (Donut)** (`src/donut_spinning.c`): Real-time surface normal lighting mapped to an ASCII luminance ramp (`.,-~:;=!*#$@`).
- **Triple Rotating Cubes** (`src/cube_spinning.c`): Three simultaneous cubes of varying sizes with independent spatial offsets.
- **Interactive 3D Rubik's Cube** (`src/rubiks_cube.c`): Fully functional puzzle rendered using true 2D scanline rasterization with ANSI colors and interactive animations.
- **3D Cube Sweeper** (`src/cube_sweeper.c`): Dual-mode 3D puzzle engine supporting both Surface Mode (continuous wrapping across 90-degree outer edges) and 3D Block Mode (volumetric excavation). Features an offline 2-tier deductive constraint solver eliminating 50/50 guesses, mouse-drag turntable camera with pitch clamping, and Voxel Contraction (shrunk clue blocks opening clear sightlines into the 3D volume).
- **3D Duck Hunt** (`src/duck_hunt.c`): Real-time raymarched arcade shooter utilizing Signed Distance Field (SDF) primitives, smooth minimum blending (`smin`), procedural terrain, mouse aiming, shotgun raycasting, and scaled block-font HUD overlays.
- **Schwarzschild Black Hole** (`src/black_hole.c`): Real-time volumetric raymarching using OpenMP, rendering curved spacetime, gravitational lensing, and relativistic Doppler beaming with TrueColor ANSI escape sequences.
- **Depth Buffering**: Custom z-buffer to prevent surface overlap and clipping artifacts.
- **Pure C**: Zero external graphic libraries or dependencies, relying only on standard C runtime (`math.h`, `stdio.h`, `string.h`, `unistd.h`).

---

## 🚀 Quick Start

### Prerequisites

A C compiler such as `gcc` or `clang` (e.g. MinGW / MSYS2 on Windows, or standard GCC on Linux / macOS), and `make`.

> 💡 **Environment Check:** Not sure if you have the required tools? Run `python doctor.py` to automatically diagnose your system and get installation instructions.

### Build with Make (Recommended)

Build all demos into the `bin/` directory at once:

```bash
make
```

Or build an individual demo:

```bash
make solar_spinning
make body_moving
make dna_spinning
make cube_spinning
make donut_spinning
make rubiks_cube
make black_hole
make cube_sweeper
make duck_hunt
```

Clean all compiled executables:

```bash
make clean
```

All compiled binaries are placed in `bin/` with identical base names (`bin/<name>.exe` on Windows, `bin/<name>` on Linux/macOS).

---

### 1. ☀️ Solar System & Planetary Orbits

```bash
# Build
make solar_spinning

# Or compile manually with gcc:
# gcc src/solar_spinning.c -o bin/solar_spinning -lm

# Run (Windows)
.\bin\solar_spinning.exe

# Run (Linux / macOS)
./bin/solar_spinning
```

**Interactive Controls:**
| Key | Action |
| :--- | :--- |
| `W` / `A` / `S` / `D` | Fly forward / left / backward / right (relative to camera direction) |
| `Q` / `E` | Fly straight up / down |
| `I` / `J` / `K` / `L` | Look around (pitch up/down, yaw left/right) |
| `[` / `]` | Halve / double simulation time speed |
| `P` | Pause / resume orbits |
| `Esc` | Exit |

---

### 2. 🤖 3D Moving Body & Camera

```bash
# Build
make body_moving

# Or compile manually with gcc:
# gcc src/body_moving.c -o bin/body_moving -lm

# Run (Windows)
.\bin\body_moving.exe

# Run (Linux / macOS)
./bin/body_moving
```

**Interactive Controls:**
| Key | Action |
| :--- | :--- |
| `W` / `A` / `S` / `D` | Walk forward / step left / backward / step right (relative to camera view) |
| `I` / `J` / `K` / `L` | Orbit third-person camera around the robot |
| `+` / `-` | Zoom in / out |
| `Esc` | Exit |

---

### 3. 🧬 DNA Double Helix (Interactive)

```bash
# Build
make dna_spinning

# Or compile manually with gcc:
# gcc src/dna_spinning.c -o bin/dna_spinning -lm

# Run (Windows)
.\bin\dna_spinning.exe

# Run (Linux / macOS)
./bin/dna_spinning
```

**Interactive Controls:**
| Key | Action |
| :--- | :--- |
| `W` / `S` | Pitch rotation (tilt up / down) |
| `A` / `D` | Yaw rotation (turn left / right) |
| `Q` / `E` | Roll rotation |
| `+` / `-` | Zoom in / out |
| `Space` | Pause / resume auto-rotation |
| `Esc` | Exit |

---

### 4. 🧊 Triple Rotating Cubes

```bash
# Build
make cube_spinning

# Or compile manually with gcc:
# gcc src/cube_spinning.c -o bin/cube_spinning -lm

# Run (Windows)
.\bin\cube_spinning.exe

# Run (Linux / macOS)
./bin/cube_spinning
```

**Interactive Controls:**
| Key | Action |
| :--- | :--- |
| `W` / `S` | Pitch rotation |
| `A` / `D` | Yaw rotation |
| `Q` / `E` | Roll rotation |
| `+` / `-` | Zoom in / out |
| `Space` | Pause / resume auto-rotation |
| `Esc` | Exit |

---

### 5. 🎲 Interactive 3D Rubik's Cube

```bash
# Build
make rubiks_cube

# Or compile manually with gcc:
# gcc src/rubiks_cube.c -o bin/rubiks_cube -lm

# Run (Windows)
.\bin\rubiks_cube.exe

# Run (Linux / macOS)
./bin/rubiks_cube
```

**Interactive Controls:**
| Key | Action |
| :--- | :--- |
| `u/d/l/r/f/b` | Standard Singmaster clockwise slice rotation |
| `U/D/L/R/F/B` (Shift) | Standard Singmaster counter-clockwise slice rotation |
| `Arrows` | Orbit Camera |
| `Space` | Instant Scramble / Reset |
| `Esc` | Exit |

---

### 6. 🕳️ Schwarzschild Black Hole & Gravitational Lensing

```bash
# Build (Requires OpenMP support in GCC)
make black_hole

# Or compile manually with gcc:
# gcc -O3 -fopenmp -Wall src/black_hole.c -o bin/black_hole -lm

# Run (Windows)
.\bin\black_hole.exe

# Run (Linux / macOS)
./bin/black_hole
```

**Interactive Controls:**
| Key | Action |
| :--- | :--- |
| `W` / `S` | Pitch camera down / up |
| `A` / `D` | Orbit camera left / right |
| `+` / `-` | Zoom in / out |
| `Esc` | Exit |

---

### 7. 🍩 3D Illuminated Donut

```bash
# Build
make donut_spinning

# Or compile manually with gcc:
# gcc src/donut_spinning.c -o bin/donut_spinning -lm

# Run (Windows)
.\bin\donut_spinning.exe

# Run (Linux / macOS)
./bin/donut_spinning
```

*Continuous autonomous dual-axis rotation with dynamic 12-level surface normal luminance calculation.*

---

### 8. 💣 3D Cube Sweeper (100% No-Guess Minesweeper)

```bash
# Build
make cube_sweeper

# Or compile manually with gcc:
# gcc -O3 -Wall -fopenmp src/cube_sweeper.c -o bin/cube_sweeper.exe -lm -fopenmp

# Run (Windows)
.\bin\cube_sweeper.exe

# Run (Linux / macOS)
./bin/cube_sweeper
```

**Interactive Controls:**
| Input | Action |
| :--- | :--- |
| `Left Mouse Drag` | Orbit turntable camera (zero roll, pitch clamped between -89 deg and +89 deg) |
| `Left Double-Click` | Reveal targeted surface tile (Surface Mode) / Dig targeted voxel (Block Mode) |
| `Right Click` | Toggle mine flag (`F`) |
| `Mouse Scroll` | Smooth camera zoom in / out |
| `W` / `A` / `S` / `D` | Keyboard camera pitch / yaw orbit |
| `M` | Toggle game mode (`Surface Mode` <-> `Block Excavation Mode`) |
| `R` | Restart puzzle with new randomized dimensions |
| `Esc` / `Q` | Exit |

---

### 9. 🦆 3D Raymarched Duck Hunt

```bash
# Build
make duck_hunt

# Or compile manually with gcc:
# gcc -O3 -Wall -fopenmp src/duck_hunt.c -o bin/duck_hunt.exe -lm -fopenmp

# Run (Windows)
.\bin\duck_hunt.exe

# Run (Linux / macOS)
./bin/duck_hunt
```

**Interactive Controls:**
| Input | Action |
| :--- | :--- |
| `Mouse Move` | Aim shotgun crosshair |
| `Left Click` | Fire shotgun blast (raycast hit detection) |
| `E` | Reload ammunition (8 shells capacity) |
| `W` / `A` / `S` / `D` | Navigate through procedural terrain |
| `Esc` | Pause game / release mouse capture |

---

> **Tip**: For the best visual experience, set your terminal window to at least **160 × 44 characters** (or maximize / fullscreen the window).

---

## Roadmap

See [FUTURE_TASKS.md](FUTURE_TASKS.md) for detailed mathematical specifications, controls, and planned simulations.

- [x] 3D Animated Hierarchical Body (`src/body_moving.c`)
- [x] DNA Double Helix visualization (`src/dna_spinning.c`)
- [x] Interactive rotation and camera controls (`WASD` / `IJKL` / zoom)
- [x] Solar System & Planetary Orbits (`src/solar_spinning.c`)
- [ ] Procedural 3D Terrain Flight Simulator (`src/fly_terrain.c`)
- [x] Black Hole & Gravitational Lensing (`src/black_hole.c`)
- [x] Interactive 3D Rubik's Cube (`src/rubiks_cube.c`)
- [x] 3D Cube Sweeper (No-Guess Deductive Engine & Dual Modes) (`src/cube_sweeper.c`)
- [x] 3D Raymarched Duck Hunt & Procedural Terrain (`src/duck_hunt.c`)
- [x] Mouse Click-and-Drag Turntable Camera Orbiting
- [ ] 3D Wave Tank & Ripple Surface (`src/wave_surface.c`)
- [ ] Jumping & Gravity mechanics in `src/body_moving.c`
- [ ] ANSI TrueColor (24-bit RGB) shading support
