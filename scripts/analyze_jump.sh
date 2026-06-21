#!/usr/bin/env bash
set -euo pipefail

out_dir="${PILGRIM_JUMP_ANALYSIS_DIR:-/tmp/pilgrim_jump_analysis}"
mkdir -p "$out_dir"

for facing in right left; do
  facing_value=1
  if [[ "$facing" == "left" ]]; then
    facing_value=-1
  fi
  for frame in 0 1 2 3; do
    ppm="$out_dir/${facing}_jump_$frame.ppm"
    png="$out_dir/${facing}_jump_$frame.png"
    rm -f "$ppm" "$png"
    PILGRIM_ISOLATE_PLAYER=1 \
    PILGRIM_FORCE_FACING="$facing_value" \
    PILGRIM_FORCE_JUMP_FRAME="$frame" \
    PILGRIM_CAPTURE="$ppm" \
    PILGRIM_CAPTURE_FRAME=10 \
    ./build/pilgrim
    sips -s format png "$ppm" --out "$png" >/dev/null 2>&1 || true
  done
done

python3 - "$out_dir" <<'PY'
import os
import sys

out_dir = sys.argv[1]
rows = []
for facing in ("right", "left"):
  for frame in range(4):
    path = os.path.join(out_dir, f"{facing}_jump_{frame}.ppm")
    with open(path, "rb") as f:
        if f.readline().strip() != b"P6":
            raise SystemExit(f"{path}: not P6")
        dims = f.readline()
        while dims.startswith(b"#"):
            dims = f.readline()
        w, h = map(int, dims.split())
        int(f.readline())
        data = f.read()

    xs = []
    ys = []
    green = 0
    for y in range(h):
        row = y * w * 3
        for x in range(w):
            i = row + x * 3
            r, g, b = data[i], data[i + 1], data[i + 2]
            if g > 180 and g > r + 80 and g > b + 80:
                green += 1
            if max(r, g, b) > 35 and not (g > 180 and g > r + 80 and g > b + 80):
                xs.append(x)
                ys.append(y)
    if not xs:
        raise SystemExit(f"frame {frame}: no sprite pixels found")
    minx, maxx = min(xs), max(xs)
    miny, maxy = min(ys), max(ys)
    cx = (minx + maxx) / 2.0
    cy = (miny + maxy) / 2.0
    margin = min(minx, miny, w - 1 - maxx, h - 1 - maxy)
    rows.append((facing, frame, minx, maxx, miny, maxy, cx, cy, margin, green))

print("facing frame minx maxx miny maxy center_x center_y margin green")
for row in rows:
    print("%s %d %d %d %d %d %.2f %.2f %d %d" % row)

greens = [r[9] for r in rows]
margins = [r[8] for r in rows]
summaries = []
for facing in ("right", "left"):
    group = [r for r in rows if r[0] == facing]
    centers_x = [r[6] for r in group]
    centers_y = [r[7] for r in group]
    summaries.append((facing, max(centers_x) - min(centers_x), max(centers_y) - min(centers_y)))
for facing, center_x_drift, center_y_drift in summaries:
    print(f"summary {facing} center_x_drift={center_x_drift:.2f} center_y_drift={center_y_drift:.2f}")
print(f"summary min_margin={min(margins)} green_pixels={sum(greens)}")

if sum(greens) != 0:
    raise SystemExit("jump analysis failed: chroma-key leak detected")
if min(margins) < 24:
    raise SystemExit("jump analysis failed: sprite is too close to a capture edge")
for facing, center_x_drift, center_y_drift in summaries:
    if center_x_drift > 36:
        raise SystemExit(f"jump analysis failed: {facing} horizontal body drift too high")
    if center_y_drift > 42:
        raise SystemExit(f"jump analysis failed: {facing} vertical body drift too high")
PY

echo "Jump analysis captures written to $out_dir"
