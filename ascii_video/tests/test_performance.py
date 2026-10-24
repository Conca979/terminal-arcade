"""Automated unit and integration tests for C-accelerated serialization,
multithreaded prefetch pipeline, and clock-synchronized frame dropping.
"""

from __future__ import annotations

import time
from pathlib import Path
from unittest.mock import patch

import cv2
import numpy as np
import pytest

from ascii_video.converter import AsciiConverter
from ascii_video.fast_serializer import (
    is_c_accelerated,
    serialize_truecolor,
    serialize_truecolor_c,
    serialize_truecolor_numpy,
)
from ascii_video.pipeline import FramePipeline
from ascii_video.player import AsciiVideoPlayer
from ascii_video.sample_generator import create_test_video


class TestPerformanceAndOptimization:
    """Tests for C acceleration, pipeline concurrency, and frame dropping."""

    @pytest.fixture
    def test_video(self, tmp_path: Path) -> Path:
        """Create a synthetic test video: 2 seconds at 10 FPS (20 frames)."""
        video_path = tmp_path / "perf_test.mp4"
        return create_test_video(video_path, duration_sec=2.0, fps=10, dimensions=(80, 60))

    def test_c_serializer_matches_numpy_fallback(self) -> None:
        """Verify C serializer matches NumPy fallback output bit-for-bit."""
        if not is_c_accelerated():
            pytest.skip("C serializer shared library not compiled")

        h, w = 40, 80
        np.random.seed(123)
        rgb = np.random.randint(0, 256, (h, w, 3), dtype=np.uint8)
        blocks = np.array([" ", "░", "▒", "▓", "█", "#", "*", "."], dtype="<U1")
        chars = np.random.choice(blocks, (h, w))

        out_c = serialize_truecolor_c(rgb, chars, threshold=8)
        out_np = serialize_truecolor_numpy(rgb, chars, threshold=8)

        assert out_c is not None
        assert out_c == out_np

    def test_c_serializer_zero_threshold_exact(self) -> None:
        """Verify threshold=0 performs exact run-length color deduplication."""
        rgb = np.array(
            [
                [[100, 150, 200], [100, 150, 200], [100, 150, 201]],
            ],
            dtype=np.uint8,
        )
        chars = np.array([["A", "B", "C"]], dtype="<U1")

        out_thresh0 = serialize_truecolor(rgb, chars, threshold=0)
        # Pixel 0 sets color, pixel 1 reuses color, pixel 2 changes by 1 so sets color again
        assert out_thresh0.count("\x1b[38;2;") == 2

        out_thresh5 = serialize_truecolor(rgb, chars, threshold=5)
        # Pixel 2 differs by only 1, so threshold 5 suppresses the escape code!
        assert out_thresh5.count("\x1b[38;2;") == 1

    def test_color_thresholding_compression_ratio(self) -> None:
        """Verify color delta thresholding compresses ANSI stream size significantly."""
        h, w = 50, 100
        # Create subtle gradient with micro-noise
        base = np.linspace(50, 200, w, dtype=np.float32)
        grid = np.tile(base, (h, 1))
        rgb = np.stack([grid, grid, grid], axis=-1)
        np.random.seed(42)
        noise = np.random.uniform(-3, 3, rgb.shape).astype(np.float32)
        noisy_rgb = np.clip(rgb + noise, 0, 255).astype(np.uint8)
        chars = np.full((h, w), "#", dtype="<U1")

        out_exact = serialize_truecolor(noisy_rgb, chars, threshold=0)
        out_thresh = serialize_truecolor(noisy_rgb, chars, threshold=8)

        # Threshold 8 should reduce escape sequence overhead by at least 40%
        compression = (len(out_exact) - len(out_thresh)) / len(out_exact)
        assert compression >= 0.40, f"Expected >= 40% compression, got {compression * 100:.1f}%"

    def test_fallback_when_c_unavailable(self) -> None:
        """Verify automatic fallback to NumPy when C library is unavailable."""
        h, w = 10, 20
        rgb = np.zeros((h, w, 3), dtype=np.uint8)
        chars = np.full((h, w), "X", dtype="<U1")

        with patch("ascii_video.fast_serializer.get_c_library", return_value=None):
            result = serialize_truecolor(rgb, chars, threshold=8)
            assert isinstance(result, str)
            assert "X" in result
            assert "\x1b[0m\x1b[K" in result

    def test_multithreaded_pipeline_ordered_delivery(self, test_video: Path) -> None:
        """Verify pipeline delivers frames in strict sequential order."""
        cap = cv2.VideoCapture(str(test_video))
        conv = AsciiConverter()
        pipeline = FramePipeline(
            cap=cap,
            converter=conv,
            dimensions_provider=lambda: (40, 20),
            num_workers=2,
            queue_size=4,
        )
        pipeline.start(start_frame=0)

        received_indices = []
        for _ in range(8):
            item = pipeline.get_frame(timeout=1.0)
            if item is not None:
                received_indices.append(item[0])

        pipeline.stop()
        cap.release()

        assert received_indices == [0, 1, 2, 3, 4, 5, 6, 7]

    def test_multithreaded_pipeline_seek_and_flush(self, test_video: Path) -> None:
        """Verify pipeline seek flushes buffers and restarts from target frame."""
        cap = cv2.VideoCapture(str(test_video))
        conv = AsciiConverter()
        pipeline = FramePipeline(
            cap=cap,
            converter=conv,
            dimensions_provider=lambda: (40, 20),
            num_workers=2,
        )
        pipeline.start(start_frame=0)

        f0 = pipeline.get_frame(timeout=1.0)
        assert f0 is not None and f0[0] == 0

        # Seek to frame 10
        pipeline.seek(10)
        f10 = pipeline.get_frame(timeout=1.0)
        assert f10 is not None and f10[0] == 10

        f11 = pipeline.get_frame(timeout=1.0)
        assert f11 is not None and f11[0] == 11

        pipeline.stop()
        cap.release()

    def test_clock_synchronized_frame_dropping(self, test_video: Path) -> None:
        """Verify clock-synchronized pacing drops lagging frames under simulated delay.

        This directly proves that slow-motion dragging is eliminated:
        even when artificial delay is injected into frame callbacks,
        the player advances with real-world wall-clock time rather than
        playing every single frame in slow motion.
        """
        player = AsciiVideoPlayer(test_video, width=40, height=20, drop_frames=True)
        rendered_frames = []

        def slow_callback(idx: int, _txt: str) -> None:
            rendered_frames.append(idx)
            # Inject artificial delay greater than frame duration (10 FPS = 0.1s frame budget)
            # 0.25s delay means each iteration takes 2.5x the frame budget!
            time.sleep(0.22)

        # Run playback for at most 4 frames under heavy artificial delay
        player.play(headless=True, max_frames=4, on_frame_rendered=slow_callback)

        assert len(rendered_frames) == 4
        # Because of the 0.22s delay per frame at 10 FPS, frames MUST have been dropped
        assert player.dropped_frames > 0
        # The frame indices rendered should skip forward (e.g. 1, 3, 5, 7 or similar)
        # rather than being strictly consecutive [1, 2, 3, 4]
        assert rendered_frames != [1, 2, 3, 4]

    def test_frame_dropping_disabled_plays_all_frames(self, test_video: Path) -> None:
        """Verify that when drop_frames=False, all frames are processed sequentially."""
        player = AsciiVideoPlayer(test_video, width=40, height=20, drop_frames=False, num_workers=0)
        rendered_frames = []

        def callback(idx: int, _txt: str) -> None:
            rendered_frames.append(idx)

        player.play(headless=True, max_frames=5, on_frame_rendered=callback)

        assert rendered_frames == [1, 2, 3, 4, 5]
        assert player.dropped_frames == 0

    def test_pipeline_flush_without_new_pos_resumes_cleanly(self, test_video: Path) -> None:
        """Verify pipeline flush without new_pos resumes prefetch from current frame."""
        player = AsciiVideoPlayer(test_video, width=40, height=20, drop_frames=False, num_workers=2)
        rendered_frames = []

        def callback(idx: int, _txt: str) -> None:
            rendered_frames.append(idx)
            if idx == 3:
                # Mid-playback flush (simulates terminal resize or mode change)
                assert player.pipeline is not None
                player.pipeline.flush()

        player.play(headless=True, max_frames=6, on_frame_rendered=callback)

        assert rendered_frames == [1, 2, 3, 4, 5, 6]

    def test_pipeline_looping_playback_continuous(self, tmp_path: Path) -> None:
        """Verify looping video plays continuously past video length without deadlock."""
        short_video = tmp_path / "short_loop.mp4"
        create_test_video(short_video, duration_sec=0.4, fps=10, dimensions=(80, 60))

        player = AsciiVideoPlayer(short_video, width=40, height=20, loop=True, num_workers=2)
        rendered_frames = []

        def callback(idx: int, _txt: str) -> None:
            rendered_frames.append(idx)

        # Play 8 frames on a 4-frame video to verify wrapping across loop boundary
        player.play(headless=True, max_frames=8, on_frame_rendered=callback)

        assert len(rendered_frames) == 8
        assert rendered_frames == [1, 2, 3, 4, 1, 2, 3, 4]

    def test_pipeline_no_last_frame_cutoff_at_eof(self, tmp_path: Path) -> None:
        """Verify all frames are rendered to completion and final frame is not truncated at EOF."""
        short_video = tmp_path / "eof_test.mp4"
        create_test_video(short_video, duration_sec=1.0, fps=10, dimensions=(80, 60))

        player = AsciiVideoPlayer(short_video, width=40, height=20, drop_frames=False, num_workers=2)
        rendered_frames = []

        player.play(headless=True, on_frame_rendered=lambda idx, _txt: rendered_frames.append(idx))

        assert len(rendered_frames) == 10
        assert rendered_frames == list(range(1, 11))

    def test_c_serializer_big_endian_parity(self) -> None:
        """Verify C serializer correctly handles big-endian Unicode glyph arrays."""
        rgb = np.zeros((2, 3, 3), dtype=np.uint8)
        chars = np.array([["A", "B", "C"], ["X", "Y", "Z"]], dtype=">U1")

        out_c = serialize_truecolor(rgb, chars, threshold=8)
        out_np = serialize_truecolor_numpy(rgb, chars, threshold=8)

        assert out_c == out_np
        assert "ABC" in out_c
        assert "XYZ" in out_c

    def test_synchronous_frame_dropping_paces_correctly(self, test_video: Path) -> None:
        """Verify synchronous fallback mode (0 workers) drops frames under simulated delay."""
        player = AsciiVideoPlayer(test_video, width=40, height=20, drop_frames=True, num_workers=0)
        rendered_frames = []

        def slow_callback(idx: int, _txt: str) -> None:
            rendered_frames.append(idx)
            # Inject delay greater than frame interval (0.22s at 10 FPS)
            time.sleep(0.22)

        player.play(headless=True, max_frames=4, on_frame_rendered=slow_callback)

        assert len(rendered_frames) == 4
        assert player.dropped_frames > 0
        assert rendered_frames != [1, 2, 3, 4]

