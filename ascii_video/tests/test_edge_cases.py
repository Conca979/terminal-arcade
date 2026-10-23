"""Edge cases, boundaries, and CLI parsing tests for ascii_video."""

from __future__ import annotations

from pathlib import Path
import numpy as np
import pytest

from ascii_video.cli import parse_args, main
from ascii_video.converter import AsciiConverter, GlyphMode
from ascii_video.player import get_key_nonblocking
from ascii_video.sample_generator import create_test_video


class TestEdgeCases:
    """Test boundary values, extreme aspect ratios, and CLI argument parsing."""

    def test_calculate_dimensions_minimal_terminal(self) -> None:
        conv = AsciiConverter()
        # Terminal size (cols=3, lines=3)
        w, h = conv.calculate_dimensions(1920, 1080, term_w=3, term_h=3)
        assert w >= 1
        assert h >= 1

    def test_calculate_dimensions_extreme_video_aspects(self) -> None:
        conv = AsciiConverter()
        # Ultra-wide panorama 5000 x 500
        w_wide, h_wide = conv.calculate_dimensions(5000, 500, term_w=120, term_h=30)
        assert w_wide <= 119
        assert h_wide <= 28
        assert w_wide > 0 and h_wide > 0

        # Ultra-tall strip 200 x 4000
        w_tall, h_tall = conv.calculate_dimensions(200, 4000, term_w=120, term_h=30)
        assert w_tall <= 119
        assert h_tall <= 28
        assert w_tall > 0 and h_tall > 0

    def test_convert_frame_uniform_black_and_white(self) -> None:
        conv = AsciiConverter(mode=GlyphMode.HYBRID, color=False)
        # Uniform pitch black
        black = np.zeros((10, 10, 3), dtype=np.uint8)
        out_b, _, _ = conv.convert_frame(black, 10, 10)
        # Clean lines without edges
        assert "|" not in out_b and "-" not in out_b

        # Uniform pure white
        white = np.full((10, 10, 3), 255, dtype=np.uint8)
        out_w, _, _ = conv.convert_frame(white, 10, 10)
        assert "|" not in out_w and "-" not in out_w

    def test_convert_frame_single_pixel_frame(self) -> None:
        conv = AsciiConverter(mode=GlyphMode.HYBRID)
        pixel = np.array([[[128, 64, 200]]], dtype=np.uint8)
        out, w, h = conv.convert_frame(pixel, 1, 1)
        assert w == 1
        assert h == 1
        assert len(out) > 0

    def test_contrast_extremes(self) -> None:
        conv = AsciiConverter()
        lum = np.array([[0.0, 128.0, 255.0]], dtype=np.float32)

        # Contrast 0.0 -> all flat 128.0
        c0 = conv.adjust_contrast(lum, 0.0)
        assert np.allclose(c0, 128.0)

        # Very high contrast 10.0 -> polarized to 0 and 255
        c_high = conv.adjust_contrast(lum, 10.0)
        assert c_high[0, 0] == 0.0
        assert c_high[0, 1] == 128.0
        assert c_high[0, 2] == 255.0

    def test_monochrome_serialization(self) -> None:
        conv = AsciiConverter()
        chars = np.array([["A", "B"], ["C", "D"]])
        mono = conv.serialize_monochrome(chars)
        assert "\x1b[38;2;" not in mono
        assert "AB" in mono
        assert "CD" in mono

    def test_cli_missing_file_returns_error(self, tmp_path: Path) -> None:
        missing = tmp_path / "not_there.mp4"
        code = main([str(missing)])
        assert code == 1

    def test_cli_parse_all_options(self) -> None:
        args = parse_args(
            [
                "input.mp4",
                "-m",
                "density",
                "-w",
                "80",
                "-H",
                "25",
                "-c",
                "1.5",
                "--edge-threshold",
                "35.0",
                "--char-aspect",
                "0.48",
                "--ramp",
                " .#",
                "--invert",
                "--no-color",
                "-l",
                "--fps",
                "24.0",
            ]
        )
        assert args.video == "input.mp4"
        assert args.mode == "density"
        assert args.width == 80
        assert args.height == 25
        assert args.contrast == 1.5
        assert args.edge_threshold == 35.0
        assert args.char_aspect == 0.48
        assert args.ramp == " .#"
        assert args.invert is True
        assert args.color is False
        assert args.loop is True
        assert args.fps == 24.0

    def test_cli_demo_mode(self, tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> None:
        # Patch play to immediately exit without full run
        monkeypatch.chdir(tmp_path)
        monkeypatch.setattr("ascii_video.player.AsciiVideoPlayer.play", lambda self: None)
        code = main(["--demo"])
        assert code == 0
        assert (tmp_path / "demo_sample.mp4").exists()

    def test_get_key_nonblocking_safe(self) -> None:
        # Calling non-blocking read in automated test environment returns None safely
        key = get_key_nonblocking()
        assert key is None or isinstance(key, str)
