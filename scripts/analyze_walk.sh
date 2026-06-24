#!/usr/bin/env bash
set -euo pipefail

out_dir="${PILGRIM_WALK_ANALYSIS_DIR:-/tmp/pilgrim_walk_analysis}"
mkdir -p "$out_dir"

walk_frames="${PILGRIM_WALK_FRAMES:-$(python3 scripts/animation_clip_info.py walk frames)}"

for facing in right left; do
  facing_value=1
  if [[ "$facing" == "left" ]]; then
    facing_value=-1
  fi
  for ((frame = 0; frame < walk_frames; frame++)); do
    ppm="$out_dir/${facing}_walk_$frame.ppm"
    png="$out_dir/${facing}_walk_$frame.png"
    rm -f "$ppm" "$png"
    PILGRIM_ISOLATE_PLAYER=1 \
    PILGRIM_SCRIPT=walk \
    PILGRIM_FORCE_FACING="$facing_value" \
    PILGRIM_FORCE_WALK_FRAME="$frame" \
    PILGRIM_CAPTURE="$ppm" \
    PILGRIM_CAPTURE_FRAME=95 \
    ./build/pilgrim
    sips -s format png "$ppm" --out "$png" >/dev/null 2>&1 || true
  done
done

python3 - "$out_dir" "$walk_frames" <<'PY'
import os
import sys

out_dir = sys.argv[1]
walk_frames = int(sys.argv[2])
rows = []
for facing in ("right", "left"):
  for frame in range(walk_frames):
    path = os.path.join(out_dir, f"{facing}_walk_{frame}.ppm")
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
    rows.append((facing, frame, minx, maxx, miny, maxy, cx, maxx - minx + 1, maxy - miny + 1, green))

print("facing frame minx maxx miny maxy center_x width height green")
for row in rows:
    print("%s %d %d %d %d %d %.2f %d %d %d" % row)

greens = [r[9] for r in rows]
summaries = []
for facing in ("right", "left"):
    group = [r for r in rows if r[0] == facing]
    centers = [r[6] for r in group]
    bases = [r[5] for r in group]
    tops = [r[4] for r in group]
    widths = [r[7] for r in group]
    summaries.append((facing, max(centers) - min(centers), max(bases) - min(bases), max(tops) - min(tops), max(widths) - min(widths)))
for facing, center_drift, baseline_drift, top_drift, width_drift in summaries:
    print(f"summary {facing} center_drift={center_drift:.2f} baseline_drift={baseline_drift} top_drift={top_drift} width_drift={width_drift}")
print(f"summary green_pixels={sum(greens)}")

if sum(greens) != 0:
    raise SystemExit("walk analysis failed: chroma-key leak detected")
for facing, center_drift, baseline_drift, _, _ in summaries:
    if baseline_drift > 3:
        raise SystemExit(f"walk analysis failed: {facing} baseline drift too high")
    if center_drift > 10:
        raise SystemExit(f"walk analysis failed: {facing} center drift too high")
for facing, _, _, top_drift, width_drift in summaries:
    if width_drift < 8:
        raise SystemExit(f"walk analysis failed: {facing} step silhouette articulation too low")
    if top_drift > 5:
        raise SystemExit(f"walk analysis failed: {facing} upper-body bob too exaggerated")
PY

echo "Walk analysis captures written to $out_dir"
