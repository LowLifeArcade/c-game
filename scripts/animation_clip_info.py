#!/usr/bin/env python3
"""Read active animation clip metadata from the plain-text clip manifest."""

from __future__ import annotations

import argparse
from pathlib import Path


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("animation")
    parser.add_argument("field", choices=("name", "sheet", "frames", "seconds"))
    parser.add_argument("--manifest", type=Path, default=Path("assets/pilgrim_animation_clips.txt"))
    args = parser.parse_args()

    clips: dict[str, tuple[str, str, int, float]] = {}
    active: dict[str, str] = {}
    for raw in args.manifest.read_text().splitlines():
        parts = raw.split()
        if not parts or parts[0].startswith("#"):
            continue
        if parts[0] == "clip" and len(parts) == 6:
            _, animation, name, sheet, frames, seconds = parts
            clips[name] = (animation, sheet, int(frames), float(seconds))
        elif parts[0] == "active" and len(parts) == 3:
            _, animation, name = parts
            active[animation] = name

    name = active[args.animation]
    animation, sheet, frames, seconds = clips[name]
    values = {"name": name, "sheet": sheet, "frames": frames, "seconds": seconds}
    print(values[args.field])


if __name__ == "__main__":
    main()
