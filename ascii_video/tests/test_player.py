"""Unit tests for ascii_video.player module."""

from __future__ import annotations

from pathlib import Path
from unittest.mock import MagicMock

import cv2
import pytest

from ascii_video.converter import AsciiConverter, GlyphMode
from ascii_video.player import AsciiVideoPlayer, get_key_nonblocking
from ascii_video.sample_generator import create_test_video


class TestAsciiVideoPlayer:
    """Test suite for AsciiVideoPlayer logic and controls."""

    @pytest.fixture
    def sample_video(self, tmp_path: Path) -> Path:
        """Create a tiny synthetic MP4 video file for testing."""
        video_path = tmp_path / "test_mini.mp4"
        return create_test_video(video_path, duration_sec=1.0, fps=10, dimensions=(80, 60))

    def test_missing_video_file_raises(self, tmp_path: Path) -> None:
        missing = tmp_path / "non_existent.mp4"
        player = AsciiVideoPlayer(missing)
        with pytest.raises(FileNotFoundError):
            player.play(headless=True)

    def test_format_time(self) -> None:
        player = AsciiVideoPlayer("dummy.mp4")
        assert player.format_time(0.0) == "00:00"
        assert player.format_time(65.4) == "01:05"
        assert player.format_time(3600) == "60:00"

    def test_build_status_line_length_bounded(self) -> None:
        player = AsciiVideoPlayer("dummy.mp4")
        player.total_frames = 100
        player.current_frame_idx = 50
        status = player.build_status_line(current_fps=29.9, term_cols=50)
        # Should be inverse video and properly truncated within 50 columns
        assert status.startswith("\x1b[7m")
        assert "\x1b[0m\x1b[K" in status

    def test_handle_keys_quit(self) -> None:
        player = AsciiVideoPlayer("dummy.mp4")
        cap = MagicMock()
        assert not player.stopped
        player.handle_key("q", cap)
        assert player.stopped

        player.stopped = False
        player.handle_key("\x1b", cap)
        assert player.stopped

    def test_handle_keys_pause(self) -> None:
        player = AsciiVideoPlayer("dummy.mp4")
        cap = MagicMock()
        assert not player.paused
        player.handle_key(" ", cap)
        assert player.paused
        player.handle_key(" ", cap)
        assert not player.paused

    def test_handle_keys_mode_toggle(self) -> None:
        conv = AsciiConverter(mode=GlyphMode.DENSITY)
        player = AsciiVideoPlayer("dummy.mp4", converter=conv)
        cap = MagicMock()

        player.handle_key("m", cap)
        assert conv.mode == GlyphMode.EDGE

        player.handle_key("M", cap)
        assert conv.mode == GlyphMode.HYBRID

        player.handle_key("m", cap)
        assert conv.mode == GlyphMode.DENSITY

    def test_handle_keys_contrast(self) -> None:
        conv = AsciiConverter(contrast=1.0)
        player = AsciiVideoPlayer("dummy.mp4", converter=conv)
        cap = MagicMock()

        player.handle_key("+", cap)
        assert round(conv.contrast, 1) == 1.1

        player.handle_key("-", cap)
        assert round(conv.contrast, 1) == 1.0

    def test_handle_keys_restart(self) -> None:
        player = AsciiVideoPlayer("dummy.mp4")
        cap = MagicMock()
        cap.read.return_value = (True, None)
        cap.get.return_value = 0
        player.current_frame_idx = 45

        player.handle_key("r", cap)
        cap.set.assert_called_with(cv2.CAP_PROP_POS_FRAMES, 0)
        assert player.current_frame_idx == 0

    def test_handle_keys_seek(self) -> None:
        player = AsciiVideoPlayer("dummy.mp4")
        player.total_frames = 300
        player.current_frame_idx = 150
        player.effective_fps = 30.0
        cap = MagicMock()
        cap.read.return_value = (True, None)
        cap.get.return_value = 0

        # Seek backward 5s = 150 frames
        player.handle_key("[", cap)
        cap.set.assert_called_with(cv2.CAP_PROP_POS_FRAMES, 0)

        # Seek forward 5s = 150 frames
        player.current_frame_idx = 50
        player.handle_key("]", cap)
        cap.set.assert_called_with(cv2.CAP_PROP_POS_FRAMES, 200)

    def test_headless_playback(self, sample_video: Path) -> None:
        player = AsciiVideoPlayer(sample_video, width=40, height=20)
        frames_received: list[int] = []

        def callback(idx: int, text: str) -> None:
            frames_received.append(idx)
            assert len(text) > 0

        player.play(headless=True, max_frames=5, on_frame_rendered=callback)
        assert len(frames_received) == 5
        assert frames_received == [1, 2, 3, 4, 5]

    def test_playback_repo_video_if_present(self) -> None:
        repo_video = Path(__file__).resolve().parent.parent.parent / "VID_20261022_230029_524.mp4"
        if not repo_video.exists():
            pytest.skip("Repo video VID_20261022_230029_524.mp4 not found")

        player = AsciiVideoPlayer(repo_video)
        frames: list[int] = []
        player.play(headless=True, max_frames=3, on_frame_rendered=lambda idx, txt: frames.append(idx))
        assert len(frames) == 3

