"""Synthetic test video generator for headless testing and visual demonstration."""

from __future__ import annotations

import math
from pathlib import Path
from typing import Tuple

import cv2
import numpy as np


def generate_synthetic_frame(
    frame_index: int,
    total_frames: int,
    width: int = 320,
    height: int = 240,
) -> np.ndarray:
    """Generate a single test frame containing color gradients and moving edges."""
    t = frame_index / max(1, total_frames)

    # 1. Background gradient (moving color waves)
    y_coords, x_coords = np.mgrid[0:height, 0:width]
    r_chan = np.sin(x_coords / 40.0 + t * 4 * math.pi) * 127 + 128
    g_chan = np.cos(y_coords / 30.0 + t * 4 * math.pi) * 127 + 128
    b_chan = np.sin((x_coords + y_coords) / 50.0) * 127 + 128

    frame = np.stack(
        [
            b_chan.astype(np.uint8),
            g_chan.astype(np.uint8),
            r_chan.astype(np.uint8),
        ],
        axis=-1,
    )

    # 2. Moving white rectangle with sharp horizontal and vertical edges
    cx = int(width / 2 + (width / 4) * math.cos(t * 2 * math.pi))
    cy = int(height / 2 + (height / 4) * math.sin(t * 2 * math.pi))
    rw, rh = 40, 30
    x1, y1 = max(0, cx - rw), max(0, cy - rh)
    x2, y2 = min(width - 1, cx + rw), min(height - 1, cy + rh)
    cv2.rectangle(frame, (x1, y1), (x2, y2), (255, 255, 255), -1)
    cv2.rectangle(frame, (x1, y1), (x2, y2), (0, 0, 0), 2)

    # 3. Rotating line to exercise diagonal edge angles ('/', '\')
    angle_rad = t * 2 * math.pi
    length = 60
    lx1 = int(cx - length * math.cos(angle_rad))
    ly1 = int(cy - length * math.sin(angle_rad))
    lx2 = int(cx + length * math.cos(angle_rad))
    ly2 = int(cy + length * math.sin(angle_rad))
    cv2.line(frame, (lx1, ly1), (lx2, ly2), (0, 255, 255), 3)

    # 4. Text overlay
    cv2.putText(
        frame,
        f"TEST FRAME {frame_index:03d}",
        (10, 25),
        cv2.FONT_HERSHEY_SIMPLEX,
        0.6,
        (255, 255, 255),
        2,
        cv2.LINE_AA,
    )

    return frame


def create_test_video(
    output_path: str | Path,
    duration_sec: float = 3.0,
    fps: int = 30,
    dimensions: Tuple[int, int] = (320, 240),
) -> Path:
    """Create a synthetic MP4 video file.

    Args:
        output_path: Target path for the MP4 file.
        duration_sec: Video length in seconds.
        fps: Frames per second.
        dimensions: (width, height) in pixels.

    Returns:
        Path to the generated video.
    """
    path = Path(output_path).resolve()
    path.parent.mkdir(parents=True, exist_ok=True)

    w, h = dimensions
    total_frames = int(max(1, duration_sec * fps))

    # FourCC codec - try mp4v, fallback to MJPG
    fourcc = cv2.VideoWriter_fourcc(*"mp4v")
    writer = cv2.VideoWriter(str(path), fourcc, float(fps), (w, h))

    if not writer.isOpened():
        fourcc = cv2.VideoWriter_fourcc(*"MJPG")
        writer = cv2.VideoWriter(str(path), fourcc, float(fps), (w, h))

    if not writer.isOpened():
        raise RuntimeError(f"Failed to create video writer at {path}")

    try:
        for idx in range(total_frames):
            frame = generate_synthetic_frame(idx, total_frames, width=w, height=h)
            writer.write(frame)
    finally:
        writer.release()

    return path


if __name__ == "__main__":
    out = Path("demo_sample.mp4")
    print(f"Generating test video to {out}...")
    create_test_video(out, duration_sec=4.0, fps=30)
    print("Done!")
