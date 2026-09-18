# Conventional Commits Guide for Spining

This document defines the standard commit message conventions used in this repository. Following these rules ensures a clean, searchable git history and enables automatic changelog generation.

---

## 1. Anatomy of a Commit Message

A conventional commit message consists of a **Header**, an optional **Body**, and optional **Footers**:

```text
<type>[optional scope]: <short description in imperative mood>

[optional detailed body explaining the 'why' and context]

[optional footer(s), e.g., BREAKING CHANGE, Closes #123]
```

### Quick Rules
1. **Header line limit**: Maximum 50 to 72 characters.
2. **Imperative mood**: Use `"add"` instead of `"added"` or `"adds"`; use `"fix"` instead of `"fixed"`.
3. **Lowercase description**: Start description with a lowercase letter (unless it begins with a proper noun or code symbol).
4. **No trailing period**: Do not end the subject line with a period (`.`).
5. **Separate body with a blank line**: If writing a body, always leave one empty line after the header.

---

## 2. Commit Types Reference

| Type | Purpose | When to Use | Example |
| :--- | :--- | :--- | :--- |
| **`feat`** | New feature | Adding gameplay features, modes, controls, rendering passes | `feat(sweeper): add voxel contraction for revealed blocks` |
| **`fix`** | Bug fix | Fixing visual glitches, math bugs, crashes, inverted controls | `fix(camera): eliminate roll by enforcing world up-vector` |
| **`refactor`**| Refactoring | Code restructuring without changing behavior or adding features | `refactor(render): decouple glyph stamping from geometry pass` |
| **`perf`** | Performance | FPS optimizations, buffer throughput, math acceleration | `perf(terminal): minimize ANSI escapes via color state tracking` |
| **`style`** | Formatting | Indentation, whitespace, code formatting (zero logic changes)| `style: apply 2-space indentation across input loop` |
| **`docs`** | Documentation | Updating README, guides, diagrams, code comments | `docs(controls): document mouse drag orbiting and keyboard shortcuts` |
| **`build`** | Build system | Makefile, GCC compiler flags, linker scripts, dependencies | `build(make): enable OpenMP multi-threading flags` |
| **`chore`** | Maintenance | `.gitignore`, repository cleanup, file reorganization | `chore(git): ignore compiled binaries in bin/` |
| **`ci`** | CI / Automation | GitHub Actions workflows, automated build scripts | `ci: add workflow to compile all demos on push` |
| **`revert`**| Reversion | Reverting a previous git commit | `revert: feat(camera): invert horizontal yaw rotation` |

---

## 3. Scopes for This Repository

Scopes are optional but strongly recommended. Place them in parentheses after the type:

* **`(sweeper)`**: Cube Sweeper gameplay logic, mine placement, cascading, rules
* **`(camera)`**: Turntable orbit, pitch/yaw rotation, projection matrices, FOV/zoom
* **`(engine)`**: Core 3D-to-2D rasterizer, Z-buffer, ID-buffer, face-culling
* **`(input)`**: Mouse dragging, double-click timing, Windows Console mode, POSIX SGR
* **`(font)`**: 3x3 and 3x5 bitmapped font glyph rendering
* **`(render)`**: ANSI escape code streaming, terminal double-buffering, `renderBuffer`
* **`(build)`**: `Makefile` discovery, compiler optimization flags

---

## 4. Breaking Changes

If a change breaks existing architecture, API contracts, or save formats:
1. Append an exclamation mark (`!`) before the colon: `feat(engine)!: switch coordinate system to Y-down`
2. Or add a `BREAKING CHANGE:` footer at the very bottom:

```text
refactor(engine)!: migrate from Euler angles to quaternion camera math

BREAKING CHANGE: `apply_camera()` now requires a quaternion pointer instead of separate pitch and yaw floats.
```

---

## 5. Ready-to-Use Templates & Examples

### Feature Example
```text
feat(sweeper): add voxel contraction to 0.3 for revealed blocks

Shrink revealed numbered blocks to a central 0.3 pedestal so open
sightlines and negative space allow players to see and click deeper
unrevealed blocks inside the 3D volume.
```

### Bug Fix Example
```text
fix(camera): clamp pitch between -89 and +89 degrees

Prevent camera from traversing over the vertical pole, eliminating
gimbal lock, orientation inversions, and disorienting flips.
```

### Performance Optimization Example
```text
perf(render): center-align buffer with leading row spaces

Replace expensive per-character terminal positioning sequences with
a precomputed line margin flushed directly in single `fwrite` block.
```

### Refactor Example
```text
refactor(input): decouple mouse drag threshold from double-click reveal

Track total displacement since mouse-down to ensure camera rotation
never misregisters as a tile excavation click.
```

---

## 6. How to Write a Commit via Command Line

### Single-Line Commit
```bash
git commit -m "feat(camera): reverse horizontal mouse drag rotation"
```

### Multi-Line Commit (Header + Body)
```bash
git commit -m "feat(sweeper): add voxel contraction to 0.3" -m "Shrink revealed clue blocks to open sightlines into the 3D interior while preserving exact cell coordinates."
```
