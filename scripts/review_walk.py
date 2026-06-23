#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
from dataclasses import dataclass
from pathlib import Path

from PIL import Image, ImageDraw


BG = (12, 14, 20)
INK = (235, 225, 190)
BASELINE = (235, 70, 70)
CENTER = (80, 170, 240)
BBOX = (235, 190, 80)
GREEN = (0, 255, 0)

EXPECTED_BEATS = [
    "1 L forward planted",
    "2 R lifts from back",
    "3 R passing hip",
    "4 R reaches down",
    "5 R forward planted",
    "6 L lifts from back",
    "7 L passing hip",
    "8 L reaches down",
]


def frame_label(index: int, count: int) -> str:
    if count == len(EXPECTED_BEATS):
        return EXPECTED_BEATS[index]
    return str(index)


@dataclass
class Frame:
    name: str
    image: Image.Image
    bounds: tuple[int, int, int, int]

    @property
    def width(self) -> int:
        x0, _, x1, _ = self.bounds
        return x1 - x0 + 1

    @property
    def height(self) -> int:
        _, y0, _, y1 = self.bounds
        return y1 - y0 + 1

    @property
    def center_x(self) -> float:
        x0, _, x1, _ = self.bounds
        return (x0 + x1) / 2.0

    @property
    def baseline_y(self) -> int:
        return self.bounds[3]

    @property
    def top_y(self) -> int:
        return self.bounds[1]


def parse_rect(value: str) -> tuple[int, int, int, int]:
    parts = [int(part.strip()) for part in value.split(",")]
    if len(parts) != 4:
        raise argparse.ArgumentTypeError("rect must be x,y,w,h")
    return parts[0], parts[1], parts[2], parts[3]


def read_ppm(path: Path) -> Image.Image:
    with path.open("rb") as f:
        if f.readline().strip() != b"P6":
            raise ValueError(f"{path} is not a P6 PPM")
        dims = f.readline()
        while dims.startswith(b"#"):
            dims = f.readline()
        w, h = map(int, dims.split())
        if int(f.readline()) != 255:
            raise ValueError(f"{path} has unsupported max color")
        data = f.read()
    return Image.frombytes("RGB", (w, h), data)


def is_key_pixel(rgb: tuple[int, int, int]) -> bool:
    r, g, b = rgb
    return g > 150 and g > r + 50 and g > b + 50


def sprite_bounds(image: Image.Image, dark_background: bool) -> tuple[int, int, int, int]:
    pixels = image.load()
    xs: list[int] = []
    ys: list[int] = []
    for y in range(image.height):
        for x in range(image.width):
            r, g, b = pixels[x, y]
            if is_key_pixel((r, g, b)):
                continue
            if dark_background and max(r, g, b) <= 35:
                continue
            if not dark_background and max(r, g, b) <= 20:
                continue
            xs.append(x)
            ys.append(y)
    if not xs:
        raise ValueError("no sprite pixels found")
    return min(xs), min(ys), max(xs), max(ys)


def crop_frame(frame: Frame, pad: int = 10) -> Image.Image:
    x0, y0, x1, y1 = frame.bounds
    x0 = max(0, x0 - pad)
    y0 = max(0, y0 - pad)
    x1 = min(frame.image.width - 1, x1 + pad)
    y1 = min(frame.image.height - 1, y1 + pad)
    return frame.image.crop((x0, y0, x1 + 1, y1 + 1))


def sprite_rgba(frame: Frame, pad: int = 0) -> tuple[Image.Image, tuple[int, int, int, int]]:
    x0, y0, x1, y1 = frame.bounds
    x0 = max(0, x0 - pad)
    y0 = max(0, y0 - pad)
    x1 = min(frame.image.width - 1, x1 + pad)
    y1 = min(frame.image.height - 1, y1 + pad)
    crop = frame.image.crop((x0, y0, x1 + 1, y1 + 1)).convert("RGBA")
    pixels = crop.load()
    for y in range(crop.height):
        for x in range(crop.width):
            r, g, b, _ = pixels[x, y]
            if is_key_pixel((r, g, b)) or max(r, g, b) <= 35:
                pixels[x, y] = (0, 0, 0, 0)
            else:
                pixels[x, y] = (r, g, b, 255)
    return crop, (x0, y0, x1, y1)


def load_pilgrim_frames(frames_dir: Path, count: int) -> list[Frame]:
    frames: list[Frame] = []
    for index in range(count):
        path = frames_dir / f"right_walk_{index}.ppm"
        image = read_ppm(path).convert("RGB")
        bounds = sprite_bounds(image, dark_background=True)
        frames.append(Frame(f"pilgrim {index}", image, bounds))
    return frames


def connected_components(mask: set[tuple[int, int]]) -> list[tuple[int, int, int, int, int]]:
    components = []
    while mask:
        start = mask.pop()
        queue = [start]
        xs = [start[0]]
        ys = [start[1]]
        for x, y in queue:
            for nx in (x - 1, x, x + 1):
                for ny in (y - 1, y, y + 1):
                    if (nx, ny) in mask:
                        mask.remove((nx, ny))
                        queue.append((nx, ny))
                        xs.append(nx)
                        ys.append(ny)
        components.append((len(queue), min(xs), min(ys), max(xs), max(ys)))
    return components


def load_reference_frames(sheet_path: Path, rect: tuple[int, int, int, int]) -> list[Frame]:
    x, y, w, h = rect
    sheet = Image.open(sheet_path).convert("RGB")
    crop = sheet.crop((x, y, x + w, y + h))
    pixels = crop.load()
    mask: set[tuple[int, int]] = set()
    for py in range(crop.height):
        for px in range(crop.width):
            rgb = pixels[px, py]
            if not is_key_pixel(rgb) and max(rgb) > 20:
                mask.add((px, py))

    components = connected_components(mask)
    sprite_components = [
        component
        for component in components
        if component[0] >= 300 and component[4] - component[2] >= 30
    ]
    sprite_components.sort(key=lambda component: component[1])

    frames = []
    for index, (_, x0, y0, x1, y1) in enumerate(sprite_components):
        # Expand a little so separated feet/whip pixels remain visible, then re-bound.
        fx0 = max(0, x0 - 2)
        fy0 = max(0, y0 - 2)
        fx1 = min(crop.width - 1, x1 + 2)
        fy1 = min(crop.height - 1, y1 + 2)
        frame_image = crop.crop((fx0, fy0, fx1 + 1, fy1 + 1))
        bounds = sprite_bounds(frame_image, dark_background=False)
        frames.append(Frame(f"richter {index}", frame_image, bounds))
    return frames


def make_contact_sheet(frames: list[Frame], out_path: Path, title: str) -> None:
    scale = 2
    crops = [crop_frame(frame, pad=10) for frame in frames]
    widths = [crop.width * scale for crop in crops]
    heights = [crop.height * scale for crop in crops]
    cell_w = max(widths) + 20
    cell_h = max(heights) + 34
    image = Image.new("RGB", (cell_w * len(frames), cell_h), BG)
    draw = ImageDraw.Draw(image)
    draw.text((8, 4), title, fill=INK)
    for index, crop in enumerate(crops):
        scaled = crop.resize((crop.width * scale, crop.height * scale), Image.Resampling.NEAREST)
        x = index * cell_w + (cell_w - scaled.width) // 2
        y = 24 + (cell_h - 24 - scaled.height) // 2
        image.paste(scaled, (x, y))
        draw.text((index * cell_w + 6, cell_h - 16), frame_label(index, len(frames)), fill=INK)
    image.save(out_path)


def normalize_frame(frame: Frame, canvas: tuple[int, int], center_x: int, baseline_y: int) -> Image.Image:
    sprite, source_rect = sprite_rgba(frame)
    x0, y0, _, y1 = source_rect
    sprite_center = (frame.bounds[0] + frame.bounds[2]) / 2.0 - x0
    sprite_baseline = frame.bounds[3] - y0
    out = Image.new("RGBA", canvas, (0, 0, 0, 0))
    paste_x = round(center_x - sprite_center)
    paste_y = round(baseline_y - sprite_baseline)
    out.alpha_composite(sprite, (paste_x, paste_y))
    return out


def make_loop(frames: list[Frame], out_path: Path, duration_ms: int = 120) -> None:
    max_w = max(frame.width for frame in frames) + 40
    max_h = max(frame.height for frame in frames) + 36
    canvas = (max_w, max_h)
    center_x = max_w // 2
    baseline_y = max_h - 18
    gif_frames = []
    for frame in frames:
        background = Image.new("RGB", canvas, BG)
        normalized = normalize_frame(frame, canvas, center_x, baseline_y)
        background.paste(normalized, (0, 0), normalized)
        draw = ImageDraw.Draw(background)
        draw.line((0, baseline_y, max_w, baseline_y), fill=BASELINE)
        draw.line((center_x, 0, center_x, max_h), fill=CENTER)
        gif_frames.append(background)
    gif_frames[0].save(out_path, save_all=True, append_images=gif_frames[1:], duration=duration_ms, loop=0)


def tint_sprite(sprite: Image.Image, color: tuple[int, int, int], alpha: int) -> Image.Image:
    tinted = Image.new("RGBA", sprite.size, (*color, 0))
    source = sprite.convert("RGBA")
    source_pixels = source.load()
    tinted_pixels = tinted.load()
    for y in range(source.height):
        for x in range(source.width):
            _, _, _, a = source_pixels[x, y]
            if a:
                tinted_pixels[x, y] = (*color, alpha)
    return tinted


def make_onion(frames: list[Frame], out_path: Path) -> None:
    max_w = max(frame.width for frame in frames) + 54
    max_h = max(frame.height for frame in frames) + 44
    canvas = (max_w, max_h)
    center_x = max_w // 2
    baseline_y = max_h - 20
    image = Image.new("RGBA", canvas, (*BG, 255))
    colors = [
        (240, 80, 80),
        (245, 130, 55),
        (230, 200, 60),
        (130, 220, 90),
        (80, 210, 180),
        (75, 170, 245),
        (135, 110, 245),
        (205, 105, 235),
        (240, 100, 170),
        (240, 240, 240),
    ]
    for index, frame in enumerate(frames):
        normalized = normalize_frame(frame, canvas, center_x, baseline_y)
        image.alpha_composite(tint_sprite(normalized, colors[index % len(colors)], 72))
    draw = ImageDraw.Draw(image)
    draw.line((0, baseline_y, max_w, baseline_y), fill=(*BASELINE, 255), width=1)
    draw.line((center_x, 0, center_x, max_h), fill=(*CENTER, 255), width=1)
    image.convert("RGB").save(out_path)


def make_guides(frames: list[Frame], out_path: Path, title: str) -> None:
    scale = 2
    crops = [crop_frame(frame, pad=12) for frame in frames]
    widths = [crop.width * scale for crop in crops]
    heights = [crop.height * scale for crop in crops]
    cell_w = max(widths) + 24
    cell_h = max(heights) + 42
    image = Image.new("RGB", (cell_w * len(frames), cell_h), BG)
    draw = ImageDraw.Draw(image)
    draw.text((8, 4), title, fill=INK)
    for index, (frame, crop) in enumerate(zip(frames, crops)):
        scaled = crop.resize((crop.width * scale, crop.height * scale), Image.Resampling.NEAREST)
        cell_x = index * cell_w
        x = cell_x + (cell_w - scaled.width) // 2
        y = 24 + (cell_h - 24 - scaled.height) // 2
        image.paste(scaled, (x, y))
        x0, y0, x1, y1 = frame.bounds
        crop_x0 = max(0, x0 - 12)
        crop_y0 = max(0, y0 - 12)
        rel_x0 = x + (x0 - crop_x0) * scale
        rel_x1 = x + (x1 - crop_x0) * scale
        rel_y0 = y + (y0 - crop_y0) * scale
        rel_y1 = y + (y1 - crop_y0) * scale
        rel_cx = int((rel_x0 + rel_x1) / 2)
        draw.rectangle((rel_x0, rel_y0, rel_x1, rel_y1), outline=BBOX)
        draw.line((cell_x, rel_y1, cell_x + cell_w, rel_y1), fill=BASELINE)
        draw.line((rel_cx, 20, rel_cx, cell_h), fill=CENTER)
        draw.text((cell_x + 6, cell_h - 16), frame_label(index, len(frames)), fill=INK)
    image.save(out_path)


def write_metrics(frames: list[Frame], out_path: Path) -> None:
    with out_path.open("w", newline="", encoding="utf-8") as f:
        writer = csv.writer(f, delimiter="\t")
        writer.writerow(["frame", "minx", "maxx", "miny", "maxy", "center_x", "baseline_y", "height", "width"])
        for index, frame in enumerate(frames):
            x0, y0, x1, y1 = frame.bounds
            writer.writerow([index, x0, x1, y0, y1, f"{frame.center_x:.2f}", y1, frame.height, frame.width])


def write_leg_plan(out_path: Path) -> None:
    with out_path.open("w", newline="", encoding="utf-8") as f:
        writer = csv.writer(f, delimiter="\t")
        writer.writerow(["frame", "expected_beat"])
        for index, beat in enumerate(EXPECTED_BEATS):
            writer.writerow([index, beat])


def main() -> None:
    parser = argparse.ArgumentParser(description="Build visual walk-animation review artifacts.")
    parser.add_argument("--frames-dir", type=Path, required=True)
    parser.add_argument("--out-dir", type=Path, required=True)
    parser.add_argument("--walk-frames", type=int, default=8)
    parser.add_argument("--reference-sheet", type=Path)
    parser.add_argument("--reference-crop", type=parse_rect, default=(0, 135, 214, 60))
    args = parser.parse_args()

    args.out_dir.mkdir(parents=True, exist_ok=True)
    pilgrim = load_pilgrim_frames(args.frames_dir, args.walk_frames)
    make_contact_sheet(pilgrim, args.out_dir / "pilgrim_contact.png", "Pilgrim forced walk frames")
    make_guides(pilgrim, args.out_dir / "pilgrim_guides.png", "Pilgrim frame guides")
    make_onion(pilgrim, args.out_dir / "pilgrim_onion.png")
    make_loop(pilgrim, args.out_dir / "pilgrim_loop.gif")
    write_metrics(pilgrim, args.out_dir / "pilgrim_metrics.tsv")
    if args.walk_frames == len(EXPECTED_BEATS):
        write_leg_plan(args.out_dir / "pilgrim_leg_plan.tsv")

    if args.reference_sheet and args.reference_sheet.exists():
        reference = load_reference_frames(args.reference_sheet, args.reference_crop)
        make_contact_sheet(reference, args.out_dir / "richter_reference_contact.png", "Richter walk reference")
        make_guides(reference, args.out_dir / "richter_reference_guides.png", "Richter reference guides")
        make_onion(reference, args.out_dir / "richter_reference_onion.png")
        make_loop(reference, args.out_dir / "richter_reference_loop.gif")
        write_metrics(reference, args.out_dir / "richter_reference_metrics.tsv")


if __name__ == "__main__":
    main()
