"""Real-time terminal video player with 24-bit TrueColor ANSI rendering."""

from __future__ import annotations

import os
import shutil
import sys
import time
from pathlib import Path
from typing import Callable, Optional

import cv2
import numpy as np

from .converter import AsciiConverter, GlyphMode
from .pipeline import FramePipeline


def init_terminal() -> None:
    """Initialize terminal for ANSI TrueColor escape sequence support."""
    try:
        import colorama

        colorama.init()
    except Exception:
        pass

    if sys.platform == "win32":
        try:
            import ctypes

            kernel32 = ctypes.windll.kernel32
            handle = kernel32.GetStdHandle(-11)  # STD_OUTPUT_HANDLE
            mode = ctypes.c_ulong()
            if kernel32.GetConsoleMode(handle, ctypes.byref(mode)):
                # ENABLE_VIRTUAL_TERMINAL_PROCESSING = 0x0004
                kernel32.SetConsoleMode(handle, mode.value | 0x0004)
        except Exception:
            pass


def get_key_nonblocking() -> Optional[str]:
    """Read keyboard input without blocking."""
    if sys.platform == "win32":
        try:
            import msvcrt

            if msvcrt.kbhit():
                ch = msvcrt.getwch()
                # Handle special key prefixes (arrows, etc.)
                if ch in ("\x00", "\xe0"):
                    scan = msvcrt.getwch()
                    if scan == "K":
                        return "LEFT"
                    if scan == "M":
                        return "RIGHT"
                    if scan == "H":
                        return "UP"
                    if scan == "P":
                        return "DOWN"
                    return None
                return ch
        except Exception:
            return None
    else:
        try:
            import select

            if select.select([sys.stdin], [], [], 0.0)[0]:
                return sys.stdin.read(1)
        except Exception:
            return None
    return None


class AsciiVideoPlayer:
    """Controls video playback, frame conversion, and terminal output."""

    def __init__(
        self,
        video_path: str | Path,
        converter: Optional[AsciiConverter] = None,
        width: Optional[int] = None,
        height: Optional[int] = None,
        fps: Optional[float] = None,
        loop: bool = False,
        drop_frames: bool = True,
        num_workers: int = 2,
    ) -> None:
        """Initialize the video player.

        Args:
            video_path: Path to the input MP4 video.
            converter: AsciiConverter instance.
            width: Fixed width override (disables dynamic terminal width).
            height: Fixed height override (disables dynamic terminal height).
            fps: Custom playback FPS override.
            loop: Whether to loop video continuously.
            drop_frames: Drop lagging frames to maintain exact real-world 1.0x playback speed.
            num_workers: Number of background conversion worker threads.
        """
        self.video_path = Path(video_path)
        self.converter = converter or AsciiConverter()
        self.override_width = width
        self.override_height = height
        self.custom_fps = fps
        self.loop = loop
        self.drop_frames = drop_frames
        self.num_workers = int(num_workers)

        self.paused = False
        self.stopped = False
        self.current_frame_idx = 0
        self.dropped_frames = 0
        self.total_frames = 0
        self.source_fps = 30.0
        self.effective_fps = 30.0
        self.video_w = 0
        self.video_h = 0
        self.duration_sec = 0.0
        self.last_frame: Optional[np.ndarray] = None
        self.pipeline: Optional[FramePipeline] = None

    def _open_capture(self) -> cv2.VideoCapture:
        """Open cv2.VideoCapture and extract metadata."""
        if not self.video_path.exists():
            raise FileNotFoundError(f"Video file not found: {self.video_path}")

        cap = cv2.VideoCapture(str(self.video_path))
        if not cap.isOpened():
            raise RuntimeError(f"Failed to open video file: {self.video_path}")

        self.video_w = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
        self.video_h = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
        cap_fps = cap.get(cv2.CAP_PROP_FPS)
        self.source_fps = cap_fps if cap_fps and cap_fps > 0 else 30.0
        self.effective_fps = self.custom_fps if self.custom_fps and self.custom_fps > 0 else self.source_fps
        self.total_frames = int(cap.get(cv2.CAP_PROP_FRAME_COUNT))
        self.duration_sec = self.total_frames / max(1.0, self.effective_fps)

        return cap

    def format_time(self, seconds: float) -> str:
        """Format seconds into MM:SS string."""
        total = int(max(0.0, seconds))
        mins = total // 60
        secs = total % 60
        return f"{mins:02d}:{secs:02d}"

    def build_status_line(self, current_fps: float, term_cols: int) -> str:
        """Build status bar string fitting current terminal width."""
        cur_sec = self.current_frame_idx / max(1.0, self.effective_fps)
        time_str = f"{self.format_time(cur_sec)}/{self.format_time(self.duration_sec)}"
        frame_str = f"{self.current_frame_idx}/{self.total_frames}"

        pause_indicator = " [PAUSED]" if self.paused else ""
        mode_str = self.converter.mode.value.capitalize()
        contrast_str = f"{self.converter.contrast:.1f}x"
        fps_str = f"{current_fps:.1f} FPS"
        drop_str = f" [Drp: {self.dropped_frames}]" if self.dropped_frames > 0 else ""

        # Assemble status segments
        info = (
            f"[{mode_str}]{pause_indicator} [{time_str} | {frame_str}] "
            f"[{fps_str}]{drop_str} [Ct: {contrast_str}] "
            f"[Space: Pause, M: Mode, +/-: Contrast, R: Restart, Q: Quit]"
        )

        # Truncate if exceeds terminal columns to avoid terminal line wrapping
        if len(info) >= term_cols:
            info = info[: max(0, term_cols - 1)]

        return f"\x1b[7m{info}\x1b[0m\x1b[K"

    def handle_key(self, key: Optional[str], cap: cv2.VideoCapture) -> bool:
        """Handle interactive keyboard input commands.

        Returns:
            True if state changed requiring an immediate frame redraw.
        """
        if not key:
            return False

        # Quit
        if key in ("q", "Q", "\x1b"):
            self.stopped = True
            return False

        # Pause / Resume
        if key == " ":
            self.paused = not self.paused
            return True

        # Mode cycling: density -> edge -> hybrid -> density
        if key in ("m", "M"):
            modes = [GlyphMode.DENSITY, GlyphMode.EDGE, GlyphMode.HYBRID]
            cur_idx = modes.index(self.converter.mode)
            self.converter.mode = modes[(cur_idx + 1) % len(modes)]
            if self.pipeline:
                self.pipeline.flush()
            return True

        # Restart
        if key in ("r", "R"):
            if self.pipeline:
                self.pipeline.seek(0)
            else:
                cap.set(cv2.CAP_PROP_POS_FRAMES, 0)
                ret, frame = cap.read()
                if ret:
                    self.last_frame = frame
            self.current_frame_idx = 0
            return True

        # Contrast adjustment
        if key in ("+", "="):
            self.converter.contrast = round(min(5.0, self.converter.contrast + 0.1), 1)
            if self.pipeline:
                self.pipeline.flush()
            return True
        if key in ("-", "_"):
            self.converter.contrast = round(max(0.1, self.converter.contrast - 0.1), 1)
            if self.pipeline:
                self.pipeline.flush()
            return True

        # Seek backward 5 seconds
        if key in ("[", "LEFT"):
            seek_frames = int(5.0 * self.effective_fps)
            target = max(0, self.current_frame_idx - seek_frames)
            if self.pipeline:
                self.pipeline.seek(target)
            else:
                cap.set(cv2.CAP_PROP_POS_FRAMES, target)
                ret, frame = cap.read()
                if ret:
                    self.last_frame = frame
            self.current_frame_idx = target
            return True

        # Seek forward 5 seconds
        if key in ("]", "RIGHT"):
            seek_frames = int(5.0 * self.effective_fps)
            target = min(max(0, self.total_frames - 1), self.current_frame_idx + seek_frames)
            if self.pipeline:
                self.pipeline.seek(target)
            else:
                cap.set(cv2.CAP_PROP_POS_FRAMES, target)
                ret, frame = cap.read()
                if ret:
                    self.last_frame = frame
            self.current_frame_idx = target
            return True

        return False

    def _render_current_frame(
        self,
        frame: np.ndarray,
        term_cols: int,
        term_lines: int,
        current_fps: float,
        headless: bool = False,
    ) -> str:
        """Render frame to terminal in-place."""
        w, h = self.converter.calculate_dimensions(
            video_w=self.video_w,
            video_h=self.video_h,
            target_w=self.override_width,
            target_h=self.override_height,
            term_w=term_cols,
            term_h=term_lines,
        )
        ascii_text, _, _ = self.converter.convert_frame(frame, w, h)
        if not headless:
            status_line = self.build_status_line(current_fps, term_cols)
            sys.stdout.write(f"\x1b[H{ascii_text}\n{status_line}\x1b[0m\x1b[K")
            sys.stdout.flush()
        return ascii_text

    def play(
        self,
        headless: bool = False,
        max_frames: Optional[int] = None,
        on_frame_rendered: Optional[Callable[[int, str], None]] = None,
    ) -> None:
        """Run the video playback loop.

        Args:
            headless: Run without writing to stdout (for testing).
            max_frames: Limit playback to N frames (for testing / benchmarking).
            on_frame_rendered: Callback invoked with (frame_index, ascii_output).
        """
        cap = self._open_capture()
        init_terminal()

        # Terminal state setup
        if not headless:
            sys.stdout.write("\x1b[?25l\x1b[2J\x1b[H")
            sys.stdout.flush()

        frame_duration = 1.0 / max(1.0, self.effective_fps)
        last_terminal_size = shutil.get_terminal_size((80, 24))
        frames_rendered = 0
        fps_tracker_time = time.perf_counter()
        fps_counter = 0
        current_measured_fps = self.effective_fps
        pause_start: Optional[float] = None

        def get_dimensions() -> Tuple[int, int]:
            cols, lines = shutil.get_terminal_size((80, 24))
            return self.converter.calculate_dimensions(
                video_w=self.video_w,
                video_h=self.video_h,
                target_w=self.override_width,
                target_h=self.override_height,
                term_w=cols,
                term_h=lines,
            )

        use_pipeline = self.num_workers > 0
        if use_pipeline:
            self.pipeline = FramePipeline(
                cap=cap,
                converter=self.converter,
                dimensions_provider=get_dimensions,
                num_workers=self.num_workers,
                loop=self.loop,
                total_frames=self.total_frames,
            )
            self.pipeline.start(start_frame=0)

        try:
            start_time = time.perf_counter()
            start_frame_idx = 0

            while not self.stopped:
                # Key processing
                key = get_key_nonblocking()
                state_changed = self.handle_key(key, cap)
                if self.stopped:
                    break

                term_cols, term_lines = shutil.get_terminal_size((80, 24))
                resized = (term_cols, term_lines) != last_terminal_size
                if resized:
                    if not headless:
                        sys.stdout.write("\x1b[2J\x1b[H")
                        sys.stdout.flush()
                    last_terminal_size = (term_cols, term_lines)
                    if self.pipeline:
                        self.pipeline.flush()

                if self.paused:
                    if pause_start is None:
                        pause_start = time.perf_counter()
                    # While paused, re-render immediately if key action changed state or terminal resized
                    if (state_changed or resized) and self.last_frame is not None:
                        self._render_current_frame(
                            self.last_frame, term_cols, term_lines, current_measured_fps, headless
                        )
                    elif not headless:
                        # Otherwise refresh status bar
                        status = self.build_status_line(current_measured_fps, term_cols)
                        sys.stdout.write(f"\x1b[{term_lines};1H{status}")
                        sys.stdout.flush()
                    time.sleep(0.04)
                    continue

                if pause_start is not None:
                    # Account for paused duration
                    start_time += (time.perf_counter() - pause_start)
                    pause_start = None

                if state_changed:
                    # Resync playback clock anchor after seek or restart
                    start_time = time.perf_counter()
                    start_frame_idx = self.current_frame_idx

                # Wall-clock pacing & target frame calculation
                now = time.perf_counter()
                elapsed = now - start_time
                target_frame = start_frame_idx + int(elapsed * self.effective_fps)

                if use_pipeline and self.pipeline:
                    # Drop lagging frames if conversion or terminal I/O fell behind
                    if self.drop_frames and target_frame > self.current_frame_idx:
                        dropped = self.pipeline.drop_to(target_frame)
                        self.dropped_frames += dropped

                    # Fetch frame from prefetch pipeline
                    frame_data = self.pipeline.get_frame(timeout=frame_duration)
                    if frame_data is None:
                        if self.pipeline.is_eof():
                            break
                        continue

                    f_idx, ascii_text, bgr_frame = frame_data
                    if self.total_frames > 0:
                        self.current_frame_idx = (f_idx % self.total_frames) + 1
                    else:
                        self.current_frame_idx = f_idx + 1
                    self.last_frame = bgr_frame
                else:
                    # Synchronous fallback with frame-dropping
                    if self.drop_frames and target_frame > self.current_frame_idx:
                        skip_count = target_frame - self.current_frame_idx
                        for _ in range(skip_count):
                            if not cap.grab():
                                break
                            self.current_frame_idx += 1
                            self.dropped_frames += 1

                    ret, frame = cap.read()
                    if not ret:
                        if self.loop:
                            cap.set(cv2.CAP_PROP_POS_FRAMES, 0)
                            self.current_frame_idx = 0
                            start_time = time.perf_counter()
                            start_frame_idx = 0
                            continue
                        else:
                            break

                    self.last_frame = frame
                    self.current_frame_idx = int(cap.get(cv2.CAP_PROP_POS_FRAMES))
                    w, h = get_dimensions()
                    ascii_text, _, _ = self.converter.convert_frame(frame, w, h)

                # Measure actual rendering FPS
                fps_counter += 1
                now = time.perf_counter()
                elapsed_fps = now - fps_tracker_time
                if elapsed_fps >= 0.5:
                    current_measured_fps = fps_counter / elapsed_fps
                    fps_counter = 0
                    fps_tracker_time = now

                # Display to terminal
                if not headless:
                    status_line = self.build_status_line(current_measured_fps, term_cols)
                    sys.stdout.write(f"\x1b[H{ascii_text}\n{status_line}\x1b[0m\x1b[K")
                    sys.stdout.flush()

                if on_frame_rendered:
                    on_frame_rendered(self.current_frame_idx, ascii_text)

                frames_rendered += 1
                if max_frames is not None and frames_rendered >= max_frames:
                    break

                # High-precision frame pacing against wall clock
                next_frame_time = start_time + (target_frame + 1 - start_frame_idx) * frame_duration
                sleep_sec = next_frame_time - time.perf_counter()
                if sleep_sec > 0.001:
                    time.sleep(sleep_sec)
                elif sleep_sec < -frame_duration * 3:
                    # If severely lagging, resync clock base
                    start_time = time.perf_counter()
                    start_frame_idx = self.current_frame_idx

        finally:
            if self.pipeline:
                self.pipeline.stop()
                self.pipeline = None
            cap.release()
            if not headless:
                # Restore cursor and terminal attributes
                sys.stdout.write("\x1b[?25h\x1b[0m\n")
                sys.stdout.flush()
