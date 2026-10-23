"""Core ASCII conversion engine for 24-bit TrueColor terminal rendering."""

from __future__ import annotations

import enum
import shutil
from typing import Tuple

import cv2
import numpy as np


class GlyphMode(str, enum.Enum):
    """Glyph mapping modes for ASCII rendering."""

    DENSITY = "density"
    EDGE = "edge"
    HYBRID = "hybrid"


RAMP_SHORT = " .:-=+*#%@"
RAMP_DETAILED = " .'`^\",:;Il!i><~+_-?][}{1)(|\\/tfjrxnuvczXYUJCLQ0OZmwqpdbkhao*#MW&8%B@$"
RAMP_BLOCKS = " ░▒▓█"


class AsciiConverter:
    """Converts image and video frames to 24-bit TrueColor ANSI ASCII strings."""

    def __init__(
        self,
        mode: GlyphMode | str = GlyphMode.HYBRID,
        contrast: float = 1.0,
        edge_threshold: float = 40.0,
        char_aspect_ratio: float = 0.5,
        ramp: str = RAMP_SHORT,
        invert: bool = False,
        color: bool = True,
    ) -> None:
        """Initialize the ASCII converter.

        Args:
            mode: Glyph mapping mode (density, edge, hybrid).
            contrast: Contrast scaling factor (1.0 = normal, >1.0 higher, <1.0 lower).
            edge_threshold: Sobel edge detection threshold (higher = fewer edges).
            char_aspect_ratio: Terminal character cell width-to-height ratio (~0.5).
            ramp: Density ramp string from dark to bright.
            invert: Invert luminance mapping.
            color: Enable 24-bit TrueColor ANSI codes.
        """
        self.mode = GlyphMode(mode) if isinstance(mode, str) else mode
        self.contrast = float(contrast)
        self.edge_threshold = float(edge_threshold)
        self.char_aspect_ratio = float(char_aspect_ratio)
        self.ramp = ramp
        self.invert = invert
        self.color = color
        self._ramp_array = np.array(list(ramp))

    def set_ramp(self, ramp: str) -> None:
        """Update density ramp."""
        self.ramp = ramp
        self._ramp_array = np.array(list(ramp))

    def calculate_dimensions(
        self,
        video_w: int,
        video_h: int,
        target_w: int | None = None,
        target_h: int | None = None,
        term_w: int | None = None,
        term_h: int | None = None,
        reserve_rows: int = 2,
    ) -> Tuple[int, int]:
        """Calculate target ASCII frame dimensions preserving physical aspect ratio.

        Args:
            video_w: Source video width in pixels.
            video_h: Source video height in pixels.
            target_w: Explicit width override.
            target_h: Explicit height override.
            term_w: Current terminal columns (queries system if None).
            term_h: Current terminal rows (queries system if None).
            reserve_rows: Number of bottom rows reserved for status bar.

        Returns:
            Tuple of (columns, rows).
        """
        if video_w <= 0 or video_h <= 0:
            return (1, 1)

        # Explicit overrides take precedence
        if target_w is not None and target_h is not None:
            return (max(1, int(target_w)), max(1, int(target_h)))

        # Physical video aspect ratio
        video_aspect = float(video_w) / float(video_h)
        # Target character grid aspect ratio compensated for non-square font cells
        # char_aspect_ratio = char_width / char_height (~0.5)
        # grid_w / grid_h = video_aspect / char_aspect_ratio
        effective_aspect = video_aspect / max(0.01, self.char_aspect_ratio)

        if target_w is not None:
            w = max(1, int(target_w))
            h = max(1, int(round(w / effective_aspect)))
            return (w, h)

        if target_h is not None:
            h = max(1, int(target_h))
            w = max(1, int(round(h * effective_aspect)))
            return (w, h)

        # Dynamic terminal fitting
        if term_w is None or term_h is None:
            cols, lines = shutil.get_terminal_size((80, 24))
            term_w = cols if term_w is None else term_w
            term_h = lines if term_h is None else term_h

        # Use term_w - 1 to ensure a full-width line never wraps automatically at terminal edge
        max_w = max(1, term_w - 1)
        max_h = max(1, term_h - max(0, reserve_rows))

        # Fit within bounding box
        # Try fitting by width first
        fit_h = int(round(max_w / effective_aspect))
        if fit_h <= max_h:
            return (max_w, max(1, fit_h))

        # Otherwise fit by height
        fit_w = int(round(max_h * effective_aspect))
        return (max(1, min(fit_w, max_w)), max_h)

    def rgb_to_luminance(self, rgb_frame: np.ndarray) -> np.ndarray:
        """Compute perceptual luminance using ITU-R BT.601 standard.

        Formula: Y = 0.299 * R + 0.587 * G + 0.114 * B
        """
        r = rgb_frame[:, :, 0].astype(np.float32)
        g = rgb_frame[:, :, 1].astype(np.float32)
        b = rgb_frame[:, :, 2].astype(np.float32)
        return 0.299 * r + 0.587 * g + 0.114 * b

    def adjust_contrast(self, luminance: np.ndarray, contrast: float) -> np.ndarray:
        """Apply contrast scaling centered at midpoint 128.0."""
        if contrast == 1.0:
            return np.clip(luminance, 0.0, 255.0)
        adjusted = (luminance - 128.0) * contrast + 128.0
        return np.clip(adjusted, 0.0, 255.0)

    def map_density_glyphs(
        self, luminance: np.ndarray, invert: bool | None = None
    ) -> np.ndarray:
        """Vectorized mapping of luminance values to density ramp glyphs."""
        should_invert = self.invert if invert is None else invert
        ramp_len = len(self._ramp_array)
        scaled = luminance * (ramp_len / 256.0)
        indices = np.clip(scaled.astype(np.int32), 0, ramp_len - 1)
        if should_invert:
            indices = (ramp_len - 1) - indices
        return self._ramp_array[indices]

    def detect_directional_edges(
        self, luminance: np.ndarray, threshold: float | None = None
    ) -> Tuple[np.ndarray, np.ndarray]:
        """Compute Sobel edge gradients and map directional angles to glyphs.

        Angle bins (degrees):
            [0, 22.5) or [157.5, 180): '|' (vertical edge)
            [22.5, 67.5):               '/' (diagonal slash)
            [67.5, 112.5):              '-' (horizontal edge)
            [112.5, 157.5):             '\\' (diagonal backslash)

        Returns:
            Tuple of (edge_chars_array, is_edge_boolean_mask).
        """
        h, w = luminance.shape
        edge_chars = np.full((h, w), " ", dtype="<U1")
        if h < 3 or w < 3:
            return (edge_chars, np.zeros((h, w), dtype=bool))

        thresh = self.edge_threshold if threshold is None else float(threshold)
        lum_u8 = luminance.astype(np.uint8)

        gx = cv2.Sobel(lum_u8, cv2.CV_32F, 1, 0, ksize=3)
        gy = cv2.Sobel(lum_u8, cv2.CV_32F, 0, 1, ksize=3)
        mag = np.hypot(gx, gy)

        angle = (np.degrees(np.arctan2(gy, gx)) + 180.0) % 180.0

        v_mask = (angle < 22.5) | (angle >= 157.5)
        d1_mask = (angle >= 22.5) & (angle < 67.5)
        h_mask = (angle >= 67.5) & (angle < 112.5)
        d2_mask = (angle >= 112.5) & (angle < 157.5)

        edge_chars[v_mask] = "|"
        edge_chars[d1_mask] = "/"
        edge_chars[h_mask] = "-"
        edge_chars[d2_mask] = "\\"

        is_edge = mag >= thresh
        return (edge_chars, is_edge)

    def serialize_truecolor(
        self, rgb_array: np.ndarray, chars_array: np.ndarray
    ) -> str:
        """Serialize RGB color array and glyph characters into ANSI TrueColor string.

        Uses run-length optimization for consecutive identical colors and appends
        line-clear escape sequence (\x1b[K) to avoid residual artifacts upon resize.
        """
        h, w, _ = rgb_array.shape
        lines: list[str] = []

        for r in range(h):
            row_rgb = rgb_array[r]
            row_chars = chars_array[r]
            line_parts: list[str] = []
            last_r, last_g, last_b = -1, -1, -1

            for c in range(w):
                cr = int(row_rgb[c, 0])
                cg = int(row_rgb[c, 1])
                cb = int(row_rgb[c, 2])

                if cr != last_r or cg != last_g or cb != last_b:
                    line_parts.append(f"\x1b[38;2;{cr};{cg};{cb}m")
                    last_r, last_g, last_b = cr, cg, cb

                line_parts.append(row_chars[c])

            # Append \x1b[K to clear any trailing characters from previous wider frames
            line_parts.append("\x1b[K")
            lines.append("".join(line_parts))

        return "\n".join(lines) + "\x1b[0m\x1b[K"

    def serialize_monochrome(self, chars_array: np.ndarray) -> str:
        """Serialize glyph characters without color escape sequences."""
        return "\n".join("".join(row) + "\x1b[K" for row in chars_array) + "\x1b[0m\x1b[K"

    def convert_frame(
        self,
        bgr_frame: np.ndarray,
        target_w: int,
        target_h: int,
    ) -> Tuple[str, int, int]:
        """Convert a BGR frame to an ASCII ANSI string buffer.

        Args:
            bgr_frame: Source BGR image array.
            target_w: Target width in characters.
            target_h: Target height in characters.

        Returns:
            Tuple of (ansi_string, target_w, target_h).
        """
        # Downsample using OpenCV INTER_AREA
        resized_bgr = cv2.resize(
            bgr_frame, (target_w, target_h), interpolation=cv2.INTER_AREA
        )
        resized_rgb = cv2.cvtColor(resized_bgr, cv2.COLOR_BGR2RGB)

        # Luminance & contrast
        lum = self.rgb_to_luminance(resized_rgb)
        lum_adj = self.adjust_contrast(lum, self.contrast)

        # Glyph mapping
        if self.mode == GlyphMode.DENSITY:
            chars = self.map_density_glyphs(lum_adj)
        elif self.mode == GlyphMode.EDGE:
            edge_chars, is_edge = self.detect_directional_edges(lum_adj)
            chars = np.where(is_edge, edge_chars, " ")
        elif self.mode == GlyphMode.HYBRID:
            density_chars = self.map_density_glyphs(lum_adj)
            edge_chars, is_edge = self.detect_directional_edges(lum_adj)
            chars = np.where(is_edge, edge_chars, density_chars)
        else:
            chars = self.map_density_glyphs(lum_adj)

        # Serialization
        if self.color:
            serialized = self.serialize_truecolor(resized_rgb, chars)
        else:
            serialized = self.serialize_monochrome(chars)

        return (serialized, target_w, target_h)
