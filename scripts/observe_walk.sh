#!/usr/bin/env bash
set -euo pipefail

out_dir="${PILGRIM_WALK_OBSERVE_DIR:-/tmp/pilgrim_walk_observe}"
mkdir -p "$out_dir"

samples=(24 30 36 42 48 54 60 66 72 78 84 90 96 102 108 114 120 126)

for facing in right left; do
  facing_value=1
  if [[ "$facing" == "left" ]]; then
    facing_value=-1
  fi
  for sample in "${samples[@]}"; do
    ppm="$out_dir/${facing}_sample_${sample}.ppm"
    png="$out_dir/${facing}_sample_${sample}.png"
    rm -f "$ppm" "$png"
    PILGRIM_ISOLATE_PLAYER=1 \
    PILGRIM_SCRIPT=walk \
    PILGRIM_FORCE_FACING="$facing_value" \
    PILGRIM_CAPTURE="$ppm" \
    PILGRIM_CAPTURE_FRAME="$sample" \
    ./build/pilgrim
    sips -s format png "$ppm" --out "$png" >/dev/null 2>&1 || true
  done
done

python3 - "$out_dir" "${samples[@]}" <<'PY'
from pathlib import Path
import sys

out_dir = Path(sys.argv[1])
samples = [int(v) for v in sys.argv[2:]]

def read_ppm(path):
    with path.open("rb") as f:
        if f.readline().strip() != b"P6":
            raise SystemExit(f"{path}: not P6")
        dims = f.readline()
        while dims.startswith(b"#"):
            dims = f.readline()
        w, h = map(int, dims.split())
        if int(f.readline()) != 255:
            raise SystemExit(f"{path}: unsupported max value")
        data = bytearray(f.read())
    return w, h, data

def sprite_bounds(w, h, data):
    xs = []
    ys = []
    green = 0
    for y in range(h):
        row = y * w * 3
        for x in range(w):
            i = row + x * 3
            r, g, b = data[i], data[i + 1], data[i + 2]
            key = g > 180 and g > r + 80 and g > b + 80
            if key:
                green += 1
            if max(r, g, b) > 35 and not key:
                xs.append(x)
                ys.append(y)
    if not xs:
        raise SystemExit("no visible sprite pixels found")
    return min(xs), max(xs), min(ys), max(ys), green

def crop(w, h, data, bounds, pad=10):
    x0, x1, y0, y1, _ = bounds
    x0 = max(0, x0 - pad)
    x1 = min(w - 1, x1 + pad)
    y0 = max(0, y0 - pad)
    y1 = min(h - 1, y1 + pad)
    cw = x1 - x0 + 1
    ch = y1 - y0 + 1
    out = bytearray(cw * ch * 3)
    for y in range(ch):
        src = ((y0 + y) * w + x0) * 3
        dst = y * cw * 3
        out[dst:dst + cw * 3] = data[src:src + cw * 3]
    return cw, ch, out

def scale2(w, h, data):
    sw = w * 2
    sh = h * 2
    out = bytearray(sw * sh * 3)
    for y in range(h):
        for x in range(w):
            pixel = data[(y * w + x) * 3:(y * w + x) * 3 + 3]
            for dy in (0, 1):
                for dx in (0, 1):
                    i = ((y * 2 + dy) * sw + x * 2 + dx) * 3
                    out[i:i + 3] = pixel
    return sw, sh, out

rows = []
images = []
for facing in ("right", "left"):
    for sample in samples:
        w, h, data = read_ppm(out_dir / f"{facing}_sample_{sample}.ppm")
        bounds = sprite_bounds(w, h, data)
        minx, maxx, miny, maxy, green = bounds
        rows.append((facing, sample, minx, maxx, miny, maxy, (minx + maxx) / 2.0, maxx - minx + 1, green))
        images.append(scale2(*crop(w, h, data, bounds)))

summary_path = out_dir / "walk_observe.tsv"
with summary_path.open("w", encoding="utf-8") as f:
    f.write("facing\tsim_frame\tminx\tmaxx\tminy\tmaxy\tcenter_x\twidth\tgreen\n")
    for row in rows:
        f.write("%s\t%d\t%d\t%d\t%d\t%d\t%.2f\t%d\t%d\n" % row)

cell_w = max(image[0] for image in images) + 16
cell_h = max(image[1] for image in images) + 16
sheet_w = cell_w * len(samples)
sheet_h = cell_h * 2
sheet = bytearray([18, 20, 25]) * (sheet_w * sheet_h)
for idx, (w, h, data) in enumerate(images):
    col = idx % len(samples)
    row = idx // len(samples)
    ox = col * cell_w + (cell_w - w) // 2
    oy = row * cell_h + (cell_h - h) // 2
    for y in range(h):
        src = y * w * 3
        dst = ((oy + y) * sheet_w + ox) * 3
        sheet[dst:dst + w * 3] = data[src:src + w * 3]

contact_ppm = out_dir / "walk_observe_contact.ppm"
with contact_ppm.open("wb") as f:
    f.write(f"P6\n{sheet_w} {sheet_h}\n255\n".encode("ascii"))
    f.write(sheet)

print("facing samples center_drift baseline_drift top_drift width_drift green")
for facing in ("right", "left"):
    group = [row for row in rows if row[0] == facing]
    centers = [row[6] for row in group]
    baselines = [row[5] for row in group]
    tops = [row[4] for row in group]
    widths = [row[7] for row in group]
    green = sum(row[8] for row in group)
    center_drift = max(centers) - min(centers)
    baseline_drift = max(baselines) - min(baselines)
    top_drift = max(tops) - min(tops)
    width_drift = max(widths) - min(widths)
    print(f"{facing} {len(group)} {center_drift:.2f} {baseline_drift} {top_drift} {width_drift} {green}")
    if green:
        raise SystemExit(f"walk observation failed: {facing} chroma-key leak detected")
    if baseline_drift > 4:
        raise SystemExit(f"walk observation failed: {facing} baseline drift too high")
    if center_drift > 10:
        raise SystemExit(f"walk observation failed: {facing} center drift too high")
    if width_drift < 8:
        raise SystemExit(f"walk observation failed: {facing} step silhouette articulation too low")
    if top_drift > 5:
        raise SystemExit(f"walk observation failed: {facing} upper-body bob too exaggerated")
PY

sips -s format png "$out_dir/walk_observe_contact.ppm" --out "$out_dir/walk_observe_contact.png" >/dev/null 2>&1 || true
echo "Walk observation captures written to $out_dir"
