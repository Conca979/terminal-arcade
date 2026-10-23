"""Command-line interface for the ASCII Video Player."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

from .converter import AsciiConverter, GlyphMode, RAMP_SHORT
from .player import AsciiVideoPlayer
from .sample_generator import create_test_video

DEFAULT_VIDEO_NAME = "VID_20261022_230029_524.mp4"


def find_default_video() -> Path | None:
    """Search for default video file in current working directory and repository root."""
    cwd_candidate = Path.cwd() / DEFAULT_VIDEO_NAME
    if cwd_candidate.exists():
        return cwd_candidate

    repo_root = Path(__file__).resolve().parent.parent / DEFAULT_VIDEO_NAME
    if repo_root.exists():
        return repo_root

    # Also search for any mp4 files in repo root
    parent = Path(__file__).resolve().parent.parent
    mp4s = list(parent.glob("*.mp4"))
    if mp4s:
        return mp4s[0]

    return None


def parse_args(args: list[str] | None = None) -> argparse.Namespace:
    """Parse CLI arguments."""
    parser = argparse.ArgumentParser(
        prog="ascii-video",
        description="Play MP4 videos directly in your terminal using 24-bit TrueColor ASCII art.",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )

    parser.add_argument(
        "video",
        nargs="?",
        default=None,
        help="Path to the MP4 video file to play.",
    )
    parser.add_argument(
        "--video",
        dest="video_flag",
        default=None,
        help="Explicit flag for video file path.",
    )
    parser.add_argument(
        "--demo",
        action="store_true",
        help="Generate and play a synthetic demonstration video.",
    )
    parser.add_argument(
        "-m",
        "--mode",
        choices=["hybrid", "density", "edge"],
        default="hybrid",
        help="Glyph mapping mode: 'density' (ramp), 'edge' (Sobel -|/\\), 'hybrid' (combined).",
    )
    parser.add_argument(
        "-w",
        "--width",
        type=int,
        default=None,
        help="Override output width in characters (disables dynamic terminal width).",
    )
    parser.add_argument(
        "-H",
        "--height",
        type=int,
        default=None,
        help="Override output height in characters (disables dynamic terminal height).",
    )
    parser.add_argument(
        "-c",
        "--contrast",
        type=float,
        default=1.0,
        help="Contrast scaling factor (1.0 = normal, >1.0 punchy, <1.0 flatter).",
    )
    parser.add_argument(
        "--edge-threshold",
        type=float,
        default=40.0,
        help="Sobel gradient magnitude threshold for edge detection.",
    )
    parser.add_argument(
        "--char-aspect",
        type=float,
        default=0.5,
        help="Monospace font cell width-to-height ratio.",
    )
    parser.add_argument(
        "--ramp",
        type=str,
        default=RAMP_SHORT,
        help="Custom density ramp string from dark to bright.",
    )
    parser.add_argument(
        "--invert",
        action="store_true",
        help="Invert density ramp luminance mapping.",
    )
    parser.add_argument(
        "--no-color",
        "--mono",
        dest="color",
        action="store_false",
        default=True,
        help="Disable 24-bit TrueColor ANSI escape sequences.",
    )
    parser.add_argument(
        "-l",
        "--loop",
        action="store_true",
        help="Loop video continuously.",
    )
    parser.add_argument(
        "--fps",
        type=float,
        default=None,
        help="Override video playback frames-per-second.",
    )

    return parser.parse_args(args)


def main(args: list[str] | None = None) -> int:
    """CLI entry point."""
    parsed = parse_args(args)

    video_target: Path | None = None

    if parsed.demo:
        demo_path = Path("demo_sample.mp4")
        if not demo_path.exists():
            print("Generating synthetic demo video (demo_sample.mp4)...")
            create_test_video(demo_path, duration_sec=4.0, fps=30)
        video_target = demo_path
    elif parsed.video_flag:
        video_target = Path(parsed.video_flag)
    elif parsed.video:
        video_target = Path(parsed.video)
    else:
        video_target = find_default_video()

    if video_target is None or not video_target.exists():
        sys.stderr.write(
            "Error: No video file specified or found.\n"
            f"Please specify a video file path or run with --demo.\n"
            f"Example: python -m ascii_video path/to/video.mp4\n"
            f"         python -m ascii_video --demo\n"
        )
        return 1

    mode_enum = GlyphMode(parsed.mode)
    converter = AsciiConverter(
        mode=mode_enum,
        contrast=parsed.contrast,
        edge_threshold=parsed.edge_threshold,
        char_aspect_ratio=parsed.char_aspect,
        ramp=parsed.ramp,
        invert=parsed.invert,
        color=parsed.color,
    )

    player = AsciiVideoPlayer(
        video_path=video_target,
        converter=converter,
        width=parsed.width,
        height=parsed.height,
        fps=parsed.fps,
        loop=parsed.loop,
    )

    try:
        player.play()
    except KeyboardInterrupt:
        pass
    except Exception as exc:
        sys.stderr.write(f"Playback error: {exc}\n")
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
