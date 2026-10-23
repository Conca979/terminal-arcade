"""Unit tests for ascii_video.converter module."""

from __future__ import annotations

import cv2
import numpy as np
import pytest

from ascii_video.converter import AsciiConverter, GlyphMode, RAMP_SHORT


class TestAsciiConverter:
    """Test suite for AsciiConverter core algorithms."""

    def test_dimensions_explicit_both(self) -> None:
        conv = AsciiConverter()
        w, h = conv.calculate_dimensions(1920, 1080, target_w=100, target_h=40)
        assert w == 100
        assert h == 40

    def test_dimensions_explicit_width_only(self) -> None:
        conv = AsciiConverter(char_aspect_ratio=0.5)
        # video_aspect = 16/9 ≈ 1.7778
        # effective_aspect = 1.7778 / 0.5 = 3.5556
        # h = round(100 / 3.5556) = 28
        w, h = conv.calculate_dimensions(1920, 1080, target_w=100)
        assert w == 100
        assert h == 28

    def test_dimensions_explicit_height_only(self) -> None:
        conv = AsciiConverter(char_aspect_ratio=0.5)
        # w = round(30 * 3.5556) = 107
        w, h = conv.calculate_dimensions(1920, 1080, target_h=30)
        assert h == 30
        assert w == 107

    def test_dimensions_dynamic_terminal_bounds(self) -> None:
        conv = AsciiConverter(char_aspect_ratio=0.5)
        # Terminal 120 x 40, reserve_rows=2 -> max_w=119, max_h=38
        w, h = conv.calculate_dimensions(1920, 1080, term_w=120, term_h=40)
        assert w <= 119
        assert h <= 38
        assert w > 0
        assert h > 0

    def test_dimensions_portrait_video(self) -> None:
        conv = AsciiConverter(char_aspect_ratio=0.5)
        # 1080 x 1920 (9:16 portrait)
        # video_aspect = 9/16 = 0.5625
        # effective_aspect = 0.5625 / 0.5 = 1.125
        # In 80x24 terminal (max_w=79, max_h=22):
        w, h = conv.calculate_dimensions(1080, 1920, term_w=80, term_h=24)
        assert w <= 79
        assert h <= 22
        assert abs((w / h) - 1.125) < 0.2

    def test_dimensions_zero_or_negative(self) -> None:
        conv = AsciiConverter()
        assert conv.calculate_dimensions(0, 1080) == (1, 1)
        assert conv.calculate_dimensions(1920, -10) == (1, 1)

    def test_rgb_to_luminance_bt601(self) -> None:
        conv = AsciiConverter()
        # Pure Red: 255 * 0.299 = 76.245
        red_frame = np.zeros((2, 2, 3), dtype=np.uint8)
        red_frame[:, :, 0] = 255
        lum_red = conv.rgb_to_luminance(red_frame)
        assert np.allclose(lum_red, 76.245, atol=0.1)

        # Pure Green: 255 * 0.587 = 149.685
        green_frame = np.zeros((2, 2, 3), dtype=np.uint8)
        green_frame[:, :, 1] = 255
        lum_green = conv.rgb_to_luminance(green_frame)
        assert np.allclose(lum_green, 149.685, atol=0.1)

        # Pure Blue: 255 * 0.114 = 29.07
        blue_frame = np.zeros((2, 2, 3), dtype=np.uint8)
        blue_frame[:, :, 2] = 255
        lum_blue = conv.rgb_to_luminance(blue_frame)
        assert np.allclose(lum_blue, 29.07, atol=0.1)

        # White: 255 * (0.299 + 0.587 + 0.114) = 255.0
        white_frame = np.full((2, 2, 3), 255, dtype=np.uint8)
        lum_white = conv.rgb_to_luminance(white_frame)
        assert np.allclose(lum_white, 255.0, atol=0.1)

    def test_adjust_contrast(self) -> None:
        conv = AsciiConverter()
        lum = np.array([[64.0, 128.0, 192.0]], dtype=np.float32)

        # Contrast 1.0 -> unchanged
        assert np.allclose(conv.adjust_contrast(lum, 1.0), lum)

        # Contrast 2.0 -> (64-128)*2+128 = 0, 128 remains 128, (192-128)*2+128 = 256 clipped to 255
        adj_high = conv.adjust_contrast(lum, 2.0)
        assert adj_high[0, 0] == 0.0
        assert adj_high[0, 1] == 128.0
        assert adj_high[0, 2] == 255.0

    def test_map_density_glyphs(self) -> None:
        conv = AsciiConverter(ramp=" .#", invert=False)
        lum = np.array([[0.0, 100.0, 255.0]], dtype=np.float32)
        mapped = conv.map_density_glyphs(lum)
        assert mapped[0, 0] == " "
        assert mapped[0, 1] == "."
        assert mapped[0, 2] == "#"

        # Inverted mapping
        mapped_inv = conv.map_density_glyphs(lum, invert=True)
        assert mapped_inv[0, 0] == "#"
        assert mapped_inv[0, 1] == "."
        assert mapped_inv[0, 2] == " "

    def test_detect_directional_edges_vertical(self) -> None:
        conv = AsciiConverter(edge_threshold=20.0)
        # Create image with vertical edge: left black (0), right white (255)
        lum = np.zeros((10, 10), dtype=np.float32)
        lum[:, 5:] = 255.0

        edge_chars, is_edge = conv.detect_directional_edges(lum)
        # Edge detected around column 4-5 with vertical bar '|'
        assert np.any(is_edge[:, 4:6])
        edge_sub = edge_chars[is_edge]
        assert "|" in edge_sub

    def test_detect_directional_edges_horizontal(self) -> None:
        conv = AsciiConverter(edge_threshold=20.0)
        # Create image with horizontal edge: top black (0), bottom white (255)
        lum = np.zeros((10, 10), dtype=np.float32)
        lum[5:, :] = 255.0

        edge_chars, is_edge = conv.detect_directional_edges(lum)
        assert np.any(is_edge[4:6, :])
        edge_sub = edge_chars[is_edge]
        assert "-" in edge_sub

    def test_detect_directional_edges_diagonals(self) -> None:
        conv = AsciiConverter(edge_threshold=15.0)
        # Backslash diagonal: row == col
        lum_bs = np.zeros((15, 15), dtype=np.float32)
        for i in range(15):
            lum_bs[i, i] = 255.0

        edge_chars, is_edge = conv.detect_directional_edges(lum_bs)
        assert np.any(is_edge)
        assert "\\" in edge_chars[is_edge]

    def test_detect_directional_edges_tiny_dimension(self) -> None:
        conv = AsciiConverter()
        lum_tiny = np.zeros((2, 2), dtype=np.float32)
        edge_chars, is_edge = conv.detect_directional_edges(lum_tiny)
        assert not np.any(is_edge)
        assert edge_chars.shape == (2, 2)

    def test_convert_frame_modes(self) -> None:
        # 20x20 BGR frame with a white horizontal stripe in the middle
        bgr = np.zeros((20, 20, 3), dtype=np.uint8)
        bgr[8:12, :] = 255

        conv_density = AsciiConverter(mode=GlyphMode.DENSITY, color=False)
        out_d, w_d, h_d = conv_density.convert_frame(bgr, 20, 20)
        assert w_d == 20
        assert h_d == 20
        assert "@" in out_d or "#" in out_d

        conv_edge = AsciiConverter(mode=GlyphMode.EDGE, color=False, edge_threshold=20.0)
        out_e, _, _ = conv_edge.convert_frame(bgr, 20, 20)
        assert "-" in out_e

        conv_hybrid = AsciiConverter(mode=GlyphMode.HYBRID, color=False, edge_threshold=20.0)
        out_h, _, _ = conv_hybrid.convert_frame(bgr, 20, 20)
        assert "-" in out_h

    def test_serialize_truecolor_run_length_optimization(self) -> None:
        conv = AsciiConverter()
        # Row 1: 5 red pixels, Row 2: 3 green, 2 blue
        rgb = np.zeros((2, 5, 3), dtype=np.uint8)
        rgb[0, :] = [255, 0, 0]
        rgb[1, :3] = [0, 255, 0]
        rgb[1, 3:] = [0, 0, 255]

        chars = np.full((2, 5), "#")
        serialized = conv.serialize_truecolor(rgb, chars)

        # Color codes should only be emitted when color changes:
        # Row 0: 1 color code for 5 chars
        # Row 1: 2 color codes (green, blue)
        # Total color escape sequences = 3
        count_esc = serialized.count("\x1b[38;2;")
        assert count_esc == 3
        # Should end with reset and line-clear
        assert serialized.endswith("\x1b[0m\x1b[K")
        assert "\x1b[K" in serialized

    def test_margin_wrap_safety(self) -> None:
        conv = AsciiConverter()
        # For standard 80-column terminal, max output width must never exceed 79 columns
        w, _ = conv.calculate_dimensions(1920, 1080, term_w=80, term_h=24)
        assert w <= 79

    def test_all_lines_have_trailing_clear(self) -> None:
        conv = AsciiConverter()
        rgb = np.zeros((3, 10, 3), dtype=np.uint8)
        chars = np.full((3, 10), "*")
        serialized = conv.serialize_truecolor(rgb, chars)
        lines = serialized.split("\n")
        assert len(lines) == 3
        for line in lines:
            assert "\x1b[K" in line

