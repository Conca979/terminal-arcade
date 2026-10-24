"""High-performance ANSI TrueColor serialization with C acceleration and NumPy fallback."""

from __future__ import annotations

import ctypes
import os
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Optional

import numpy as np

_LIB: Optional[ctypes.CDLL] = None
_LIB_TRIED = False


def _get_lib_path() -> Path:
    """Return the platform-specific shared library path."""
    current_dir = Path(__file__).resolve().parent
    if sys.platform == "win32":
        return current_dir / "fast_serializer.dll"
    return current_dir / "fast_serializer.so"


def _compile_c_library() -> Optional[Path]:
    """Attempt to compile fast_serializer.c using gcc if available."""
    c_source = Path(__file__).resolve().parent / "fast_serializer.c"
    if not c_source.exists():
        return None

    gcc_path = shutil.which("gcc")
    if not gcc_path:
        return None

    lib_path = _get_lib_path()
    try:
        cmd = [
            gcc_path,
            "-O3",
            "-shared",
            "-fPIC",
            "-o",
            str(lib_path),
            str(c_source),
        ]
        res = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=10)
        if res.returncode == 0 and lib_path.exists():
            return lib_path
    except Exception:
        pass
    return None


def get_c_library() -> Optional[ctypes.CDLL]:
    """Load or compile and load the C serializer shared library."""
    global _LIB, _LIB_TRIED
    if _LIB is not None:
        return _LIB
    if _LIB_TRIED:
        return None

    _LIB_TRIED = True
    lib_path = _get_lib_path()

    if not lib_path.exists():
        compiled = _compile_c_library()
        if compiled is not None:
            lib_path = compiled

    if lib_path.exists():
        try:
            lib = ctypes.CDLL(str(lib_path))
            lib.serialize_truecolor.restype = ctypes.c_int
            lib.serialize_truecolor.argtypes = [
                ctypes.c_void_p,  # rgb (uint8*)
                ctypes.c_void_p,  # chars (uint32*)
                ctypes.c_int,     # height
                ctypes.c_int,     # width
                ctypes.c_int,     # threshold
                ctypes.c_char_p,  # out buffer
                ctypes.c_int,     # max_len
            ]
            _LIB = lib
            return _LIB
        except Exception:
            _LIB = None

    return None


def is_c_accelerated() -> bool:
    """Check if the compiled C serializer is currently available."""
    return get_c_library() is not None


def serialize_truecolor_c(
    rgb_array: np.ndarray,
    chars_array: np.ndarray,
    threshold: int = 8,
) -> Optional[str]:
    """Serialize RGB and characters using the compiled C shared library.

    Args:
        rgb_array: 3D uint8 ndarray of shape (H, W, 3).
        chars_array: 2D ndarray of shape (H, W) containing glyphs.
        threshold: Color delta threshold (0 = exact match).

    Returns:
        ANSI TrueColor formatted string, or None if C serializer unavailable.
    """
    lib = get_c_library()
    if lib is None:
        return None

    h, w, _ = rgb_array.shape
    if h == 0 or w == 0:
        return "\x1b[0m\x1b[K"

    # Ensure contiguous C-ordered memory buffers
    if not rgb_array.flags["C_CONTIGUOUS"] or rgb_array.dtype != np.uint8:
        rgb_c = np.ascontiguousarray(rgb_array, dtype=np.uint8)
    else:
        rgb_c = rgb_array

    # View glyph array as contiguous uint32 array of Unicode code points
    if chars_array.dtype != np.dtype("<U1") and chars_array.dtype != np.dtype(">U1"):
        chars_str = chars_array.astype("<U1")
        chars_u32 = np.ascontiguousarray(chars_str.view(np.uint32))
    else:
        chars_u32 = np.ascontiguousarray(chars_array.view(np.uint32))

    # Output buffer sizing: worst-case 24 bytes per character + line headers + safety margin
    buf_size = max(4096, h * w * 28 + h * 8 + 1024)
    out_buf = ctypes.create_string_buffer(buf_size)

    written = lib.serialize_truecolor(
        rgb_c.ctypes.data_as(ctypes.c_void_p),
        chars_u32.ctypes.data_as(ctypes.c_void_p),
        int(h),
        int(w),
        int(threshold),
        out_buf,
        int(buf_size),
    )

    if written < 0:
        # Buffer was too small; reallocate with double size and retry once
        buf_size *= 2
        out_buf = ctypes.create_string_buffer(buf_size)
        written = lib.serialize_truecolor(
            rgb_c.ctypes.data_as(ctypes.c_void_p),
            chars_u32.ctypes.data_as(ctypes.c_void_p),
            int(h),
            int(w),
            int(threshold),
            out_buf,
            int(buf_size),
        )
        if written < 0:
            return None

    return out_buf.raw[:written].decode("utf-8")


def serialize_truecolor_numpy(
    rgb_array: np.ndarray,
    chars_array: np.ndarray,
    threshold: int = 8,
) -> str:
    """Pure Python/NumPy TrueColor serialization with color delta thresholding.

    Uses Python list unpacking to bypass numpy scalar indexing overhead.
    """
    h, w, _ = rgb_array.shape
    if h == 0 or w == 0:
        return "\x1b[0m\x1b[K"

    thresh_sq = int(threshold) * int(threshold)
    rgb_list = rgb_array.tolist()
    chars_list = chars_array.tolist()
    lines: list[str] = []

    for r in range(h):
        row_rgb = rgb_list[r]
        row_chars = chars_list[r]
        line_parts: list[str] = []
        last_r, last_g, last_b = -1000, -1000, -1000

        for c in range(w):
            cr, cg, cb = row_rgb[c]
            dr = cr - last_r
            dg = cg - last_g
            db = cb - last_b

            if threshold <= 0:
                change = (dr != 0 or dg != 0 or db != 0)
            else:
                change = (dr * dr + dg * dg + db * db > thresh_sq)

            if change:
                line_parts.append(f"\x1b[38;2;{cr};{cg};{cb}m")
                last_r, last_g, last_b = cr, cg, cb

            line_parts.append(row_chars[c])

        line_parts.append("\x1b[K")
        lines.append("".join(line_parts))

    return "\n".join(lines) + "\x1b[0m\x1b[K"


def serialize_truecolor(
    rgb_array: np.ndarray,
    chars_array: np.ndarray,
    threshold: int = 8,
) -> str:
    """Serialize RGB colors and glyphs to ANSI TrueColor, using C if available."""
    c_res = serialize_truecolor_c(rgb_array, chars_array, threshold=threshold)
    if c_res is not None:
        return c_res
    return serialize_truecolor_numpy(rgb_array, chars_array, threshold=threshold)
