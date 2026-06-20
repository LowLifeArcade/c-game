#!/usr/bin/env bash
set -euo pipefail

out_dir="${PILGRIM_WALK_ANALYSIS_DIR:-/tmp/pilgrim_walk_analysis}"
mkdir -p "$out_dir"

for frame in 0 1 2 3 4 5 6 7 8 9; do
  ppm="$out_dir/walk_$frame.ppm"
  png="$out_dir/walk_$frame.png"
  rm -f "$ppm" "$png"
  PILGRIM_ISOLATE_PLAYER=1 \
  PILGRIM_SCRIPT=walk \
  PILGRIM_FORCE_WALK_FRAME="$frame" \
  PILGRIM_CAPTURE="$ppm" \
  PILGRIM_CAPTURE_FRAME=95 \
  ./build/pilgrim
  sips -s format png "$ppm" --out "$png" >/dev/null 2>&1 || true
done

python3 - "$out_dir" <<'PY'
import os
import sys

out_dir = sys.argv[1]
rows = []
for frame in range(10):
    path = os.path.join(out_dir, f"walk_{frame}.ppm")
    with open(path, "rb") as f:
        if f.readline().strip() != b"P6":
            raise SystemExit(f"{path}: not P6")
        dims = f.readline()
        while dims.startswith(b"#"):
            dims = f.readline()
        w, h = map(int, dims.split())
        maxv = int(f.readline())
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
            # isolate mode clears to a very dark blue/black. Treat visible lit pixels as sprite.
            if max(r, g, b) > 35 and not (g > 180 and g > r + 80 and g > b + 80):
                xs.append(x)
                ys.append(y)
    if not xs:
        raise SystemExit(f"frame {frame}: no sprite pixels found")
    minx, maxx = min(xs), max(xs)
    miny, maxy = min(ys), max(ys)
    cx = (minx + maxx) / 2.0
    rows.append((frame, minx, maxx, miny, maxy, cx, green))

print("frame minx maxx miny maxy center_x green")
for row in rows:
    print("%d %d %d %d %d %.2f %d" % row)

centers = [r[5] for r in rows]
bases = [r[4] for r in rows]
tops = [r[3] for r in rows]
greens = [r[6] for r in rows]
center_drift = max(centers) - min(centers)
baseline_drift = max(bases) - min(bases)
top_drift = max(tops) - min(tops)
print(f"summary center_drift={center_drift:.2f} baseline_drift={baseline_drift} top_drift={top_drift} green_pixels={sum(greens)}")

if sum(greens) != 0:
    raise SystemExit("walk analysis failed: chroma-key leak detected")
if baseline_drift > 3:
    raise SystemExit("walk analysis failed: baseline drift too high")
if center_drift > 10:
    raise SystemExit("walk analysis failed: center drift too high")
PY

echo "Walk analysis captures written to $out_dir"
