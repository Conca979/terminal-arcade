"""Multithreaded producer-consumer prefetch pipeline for ASCII video playback."""

from __future__ import annotations

import queue
import threading
import time
from typing import Callable, Optional, Tuple

import cv2
import numpy as np

from .converter import AsciiConverter


class FramePipeline:
    """Decoupled multithreaded frame decoding and ASCII conversion pipeline."""

    def __init__(
        self,
        cap: cv2.VideoCapture,
        converter: AsciiConverter,
        dimensions_provider: Callable[[], Tuple[int, int]],
        num_workers: int = 2,
        queue_size: int = 4,
        loop: bool = False,
    ) -> None:
        """Initialize prefetch pipeline.

        Args:
            cap: OpenCV VideoCapture instance.
            converter: AsciiConverter instance.
            dimensions_provider: Callable returning current (target_w, target_h).
            num_workers: Number of concurrent frame conversion worker threads.
            queue_size: Capacity of raw and converted frame buffers.
            loop: Whether to loop video capture to beginning upon EOF.
        """
        self.cap = cap
        self.converter = converter
        self.dimensions_provider = dimensions_provider
        self.num_workers = max(1, int(num_workers))
        self.queue_size = max(2, int(queue_size))
        self.loop = loop

        self._stopped = False
        self._eof = False
        self._lock = threading.Lock()
        self._cond = threading.Condition(self._lock)

        self._decode_queue: queue.Queue[Optional[Tuple[int, np.ndarray, int, int, int]]] = queue.Queue(
            maxsize=self.queue_size
        )
        self._results: dict[int, Tuple[str, np.ndarray]] = {}
        self._next_frame_idx = 0
        self._reader_frame_idx = 0
        self._flush_generation = 0

        self._reader_thread: Optional[threading.Thread] = None
        self._worker_threads: list[threading.Thread] = []

    def start(self, start_frame: int = 0) -> None:
        """Start reader and worker threads."""
        self._stopped = False
        self._eof = False
        self._next_frame_idx = start_frame
        self._reader_frame_idx = start_frame

        self._reader_thread = threading.Thread(
            target=self._reader_loop, name="AsciiPipeline-Reader", daemon=True
        )
        self._reader_thread.start()

        self._worker_threads = []
        for i in range(self.num_workers):
            t = threading.Thread(
                target=self._worker_loop,
                name=f"AsciiPipeline-Worker-{i}",
                daemon=True,
            )
            t.start()
            self._worker_threads.append(t)

    def _reader_loop(self) -> None:
        """Reader thread: decodes frames from VideoCapture and feeds decode_queue."""
        while not self._stopped:
            with self._cond:
                gen = self._flush_generation
                # If reader is significantly behind consumer target, fast-forward using grab()
                target_ahead = self._next_frame_idx - self._reader_frame_idx
                if target_ahead > 2:
                    for _ in range(target_ahead - 1):
                        if not self.cap.grab():
                            break
                        self._reader_frame_idx += 1
                        if self._flush_generation != gen or self._stopped:
                            break
                    if self._flush_generation != gen or self._stopped:
                        continue

            ret, frame = self.cap.read()
            if not ret:
                with self._cond:
                    if self._flush_generation != gen or self._stopped:
                        continue
                    if self.loop:
                        self.cap.set(cv2.CAP_PROP_POS_FRAMES, 0)
                        self._reader_frame_idx = 0
                        continue
                    else:
                        self._eof = True
                        self._cond.notify_all()
                        break

            with self._cond:
                if self._flush_generation != gen or self._stopped:
                    continue
                frame_idx = self._reader_frame_idx
                self._reader_frame_idx += 1
                target_w, target_h = self.dimensions_provider()

            # Push to decode queue with generation tag
            while not self._stopped:
                try:
                    self._decode_queue.put((frame_idx, frame, target_w, target_h, gen), timeout=0.05)
                    break
                except queue.Full:
                    with self._cond:
                        if self._flush_generation != gen or self._stopped:
                            break

    def _worker_loop(self) -> None:
        """Worker thread: converts raw frames to ASCII strings concurrently."""
        while not self._stopped:
            try:
                item = self._decode_queue.get(timeout=0.05)
            except queue.Empty:
                continue

            if item is None:
                break

            frame_idx, frame, target_w, target_h, gen = item

            with self._cond:
                if gen != self._flush_generation or self._stopped:
                    self._decode_queue.task_done()
                    continue

            # Heavy conversion step (releases GIL in OpenCV and C serializer)
            ascii_text, _, _ = self.converter.convert_frame(frame, target_w, target_h)

            with self._cond:
                if gen == self._flush_generation and not self._stopped:
                    self._results[frame_idx] = (ascii_text, frame)
                    self._cond.notify_all()

            self._decode_queue.task_done()

    def get_frame(self, timeout: float = 0.1) -> Optional[Tuple[int, str, np.ndarray]]:
        """Retrieve the next sequential frame.

        Returns:
            Tuple of (frame_index, ascii_text, bgr_frame), or None on timeout/EOF.
        """
        end_time = time.perf_counter() + timeout

        with self._cond:
            while not self._stopped:
                if self._next_frame_idx in self._results:
                    ascii_text, frame = self._results.pop(self._next_frame_idx)
                    idx = self._next_frame_idx
                    self._next_frame_idx += 1
                    return (idx, ascii_text, frame)

                if self._eof and not self._results:
                    return None

                remaining = end_time - time.perf_counter()
                if remaining <= 0:
                    return None

                self._cond.wait(timeout=min(0.01, remaining))

        return None

    def drop_to(self, target_frame_idx: int) -> int:
        """Drop lagging frames up to target_frame_idx.

        Returns:
            Count of frames successfully dropped.
        """
        dropped = 0
        with self._cond:
            if target_frame_idx <= self._next_frame_idx:
                return 0

            # Discard any buffered frames older than target_frame_idx
            keys_to_drop = [k for k in self._results.keys() if k < target_frame_idx]
            for k in keys_to_drop:
                del self._results[k]

            dropped = target_frame_idx - self._next_frame_idx
            self._next_frame_idx = target_frame_idx
            self._cond.notify_all()

        return dropped

    def flush(self, new_pos: Optional[int] = None) -> None:
        """Clear all buffered frames and restart prefetching immediately."""
        with self._cond:
            self._flush_generation += 1
            # Drain decode queue
            while not self._decode_queue.empty():
                try:
                    self._decode_queue.get_nowait()
                    self._decode_queue.task_done()
                except (queue.Empty, ValueError):
                    break

            self._results.clear()

            if new_pos is not None:
                self.cap.set(cv2.CAP_PROP_POS_FRAMES, max(0, new_pos))
                self._next_frame_idx = max(0, new_pos)
                self._reader_frame_idx = max(0, new_pos)

            self._eof = False
            self._cond.notify_all()

    def seek(self, target_frame: int) -> None:
        """Seek to a specific target frame index."""
        self.flush(new_pos=target_frame)

    def is_eof(self) -> bool:
        """Check if pipeline has reached end-of-file and output is exhausted."""
        with self._cond:
            return self._eof and len(self._results) == 0

    def stop(self) -> None:
        """Stop all background worker threads and join."""
        with self._cond:
            self._stopped = True
            self._flush_generation += 1
            self._cond.notify_all()

        # Drain decode queue to unblock any waiting worker
        while not self._decode_queue.empty():
            try:
                self._decode_queue.get_nowait()
                self._decode_queue.task_done()
            except (queue.Empty, ValueError):
                break

        # Send poison pills
        for _ in range(self.num_workers):
            try:
                self._decode_queue.put_nowait(None)
            except (queue.Full, ValueError):
                break

        if self._reader_thread and self._reader_thread.is_alive():
            self._reader_thread.join(timeout=1.0)

        for t in self._worker_threads:
            if t.is_alive():
                t.join(timeout=1.0)

        self._worker_threads.clear()
        self._results.clear()
