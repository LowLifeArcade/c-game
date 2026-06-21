#!/usr/bin/env bash
set -euo pipefail

out_dir="${PILGRIM_IDLE_ANALYSIS_DIR:-/tmp/pilgrim_idle_analysis}"
mkdir -p "$out_dir"

for facing in right left; do
  facing_value=1
  if [[ "$facing" == "left" ]]; then
    facing_value=-1
  fi
  for frame in 0 1 2 3 4 5; do
    ppm="$out_dir/${facing}_idle_$frame.ppm"
    png="$out_dir/${facing}_idle_$frame.png"
    rm -f "$ppm" "$png"
    PILGRIM_ISOLATE_PLAYER=1 \
    PILGRIM_FORCE_FACING="$facing_value" \
    PILGRIM_FORCE_IDLE_FRAME="$frame" \
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
  for frame in range(6):
    path = os.path.join(out_dir, f"{facing}_idle_{frame}.ppm")
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
            is_green = g > 180 and g > r + 80 and g > b + 80
            if is_green:
                green += 1
            if max(r, g, b) > 35 and not is_green:
                xs.append(x)
                ys.append(y)
    if not xs:
        raise SystemExit(f"frame {frame}: no sprite pixels found")
    minx, maxx = min(xs), max(xs)
    miny, maxy = min(ys), max(ys)
    sx = sorted(xs)
    body_minx = sx[int(len(sx) * 0.05)]
    body_maxx = sx[int(len(sx) * 0.95)]
    body_cx = (body_minx + body_maxx) / 2.0
    rows.append((facing, frame, minx, maxx, miny, maxy, body_cx, green))

print("facing frame minx maxx miny maxy body_center_x green")
for row in rows:
    print("%s %d %d %d %d %d %.2f %d" % row)

greens = [r[7] for r in rows]
summaries = []
for facing in ("right", "left"):
    group = [r for r in rows if r[0] == facing]
    centers = [r[6] for r in group]
    bases = [r[5] for r in group]
    tops = [r[4] for r in group]
    summaries.append((facing, max(centers) - min(centers), max(bases) - min(bases), max(tops) - min(tops)))
for facing, center_drift, baseline_drift, top_drift in summaries:
    print(f"summary {facing} center_drift={center_drift:.2f} baseline_drift={baseline_drift} top_drift={top_drift}")
print(f"summary green_pixels={sum(greens)}")

if sum(greens) != 0:
    raise SystemExit("idle analysis failed: chroma-key leak detected")
for facing, center_drift, baseline_drift, _ in summaries:
    if baseline_drift > 2:
        raise SystemExit(f"idle analysis failed: {facing} planted feet baseline drift too high")
    if center_drift > 8:
        raise SystemExit(f"idle analysis failed: {facing} body center drift too high")
PY

echo "Idle analysis captures written to $out_dir"
