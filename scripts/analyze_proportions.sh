#!/usr/bin/env bash
set -euo pipefail

out_dir="${PILGRIM_PROPORTION_ANALYSIS_DIR:-/tmp/pilgrim_proportion_analysis}"
mkdir -p "$out_dir"

capture() {
  local name="$1"
  local ppm="$out_dir/$name.ppm"
  local png="$out_dir/$name.png"
  shift
  rm -f "$ppm" "$png"
  env PILGRIM_ISOLATE_PLAYER=1 PILGRIM_CAPTURE="$ppm" PILGRIM_CAPTURE_FRAME=10 "$@" ./build/pilgrim
  sips -s format png "$ppm" --out "$png" >/dev/null 2>&1 || true
}

for facing in right left; do
  facing_value=1
  if [[ "$facing" == "left" ]]; then
    facing_value=-1
  fi

  for frame in 0 1 2 3 4 5; do
    capture "${facing}_idle_$frame" PILGRIM_FORCE_FACING="$facing_value" PILGRIM_FORCE_IDLE_FRAME="$frame"
  done

  for frame in 0 1 2 3 4 5 6 7 8 9; do
    capture "${facing}_walk_$frame" PILGRIM_FORCE_FACING="$facing_value" PILGRIM_SCRIPT=walk PILGRIM_FORCE_WALK_FRAME="$frame"
  done

  for frame in 0 1 2 3; do
    capture "${facing}_jump_$frame" PILGRIM_FORCE_FACING="$facing_value" PILGRIM_FORCE_JUMP_FRAME="$frame"
  done

  capture "${facing}_slash" PILGRIM_FORCE_FACING="$facing_value" PILGRIM_SCRIPT=attack PILGRIM_CAPTURE_FRAME=28
  capture "${facing}_dash" PILGRIM_FORCE_FACING="$facing_value" PILGRIM_SCRIPT=dash PILGRIM_CAPTURE_FRAME=34
done

python3 - "$out_dir" <<'PY'
import os
import statistics
import sys

out_dir = sys.argv[1]

def bounds(name):
    path = os.path.join(out_dir, f"{name}.ppm")
    with open(path, "rb") as f:
        if f.readline().strip() != b"P6":
            raise SystemExit(f"{name}: not P6")
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
            key = g > 180 and g > r + 80 and g > b + 80
            if key:
                green += 1
            if max(r, g, b) > 35 and not key:
                xs.append(x)
                ys.append(y)
    if not xs:
        raise SystemExit(f"{name}: no visible sprite pixels")
    minx, maxx = min(xs), max(xs)
    miny, maxy = min(ys), max(ys)
    sx = sorted(xs)
    body_minx = sx[int(len(sx) * 0.05)]
    body_maxx = sx[int(len(sx) * 0.95)]
    return {
        "name": name,
        "w": maxx - minx + 1,
        "h": maxy - miny + 1,
        "minx": minx,
        "maxx": maxx,
        "miny": miny,
        "maxy": maxy,
        "cx": (minx + maxx) / 2.0,
        "body_cx": (body_minx + body_maxx) / 2.0,
        "cy": (miny + maxy) / 2.0,
        "green": green,
    }

groups = {}
for facing in ("right", "left"):
    groups[f"{facing}_idle"] = [bounds(f"{facing}_idle_{i}") for i in range(6)]
    groups[f"{facing}_walk"] = [bounds(f"{facing}_walk_{i}") for i in range(10)]
    groups[f"{facing}_jump"] = [bounds(f"{facing}_jump_{i}") for i in range(4)]
    groups[f"{facing}_action"] = [bounds(f"{facing}_slash"), bounds(f"{facing}_dash")]

print("name width height center_x body_center_x center_y baseline green")
for group in groups.values():
    for r in group:
        print(f"{r['name']} {r['w']} {r['h']} {r['cx']:.2f} {r['body_cx']:.2f} {r['cy']:.2f} {r['maxy']} {r['green']}")

all_green = sum(r["green"] for group in groups.values() for r in group)
if all_green:
    raise SystemExit("proportion analysis failed: chroma-key leak detected")
for facing in ("right", "left"):
    idle_heights = [r["h"] for r in groups[f"{facing}_idle"]]
    walk_heights = [r["h"] for r in groups[f"{facing}_walk"]]
    jump_heights = [r["h"] for r in groups[f"{facing}_jump"]]
    idle_centers = [r["body_cx"] for r in groups[f"{facing}_idle"]]
    walk_centers = [r["cx"] for r in groups[f"{facing}_walk"]]
    jump_centers = [r["cx"] for r in groups[f"{facing}_jump"]]
    walk_median_h = statistics.median(walk_heights)
    idle_median_h = statistics.median(idle_heights)
    jump_max_h = max(jump_heights)
    jump_min_h = min(jump_heights)
    print(
        "summary "
        f"{facing} "
        f"idle_median_h={idle_median_h:.1f} "
        f"walk_median_h={walk_median_h:.1f} "
        f"jump_min_h={jump_min_h} "
        f"jump_max_h={jump_max_h} "
        f"idle_center_drift={max(idle_centers)-min(idle_centers):.2f} "
        f"walk_center_drift={max(walk_centers)-min(walk_centers):.2f} "
        f"jump_center_drift={max(jump_centers)-min(jump_centers):.2f}"
    )

    if abs(idle_median_h - walk_median_h) > 8:
        raise SystemExit(f"proportion analysis failed: {facing} idle and walk scales diverge")
    if max(idle_heights) - min(idle_heights) > 8:
        raise SystemExit(f"proportion analysis failed: {facing} idle breathing changes character height too much")
    if max(idle_centers) - min(idle_centers) > 5:
        raise SystemExit(f"proportion analysis failed: {facing} idle breathing shifts horizontally")
    if max(walk_centers) - min(walk_centers) > 5:
        raise SystemExit(f"proportion analysis failed: {facing} walk shifts horizontally")
    if max(jump_centers) - min(jump_centers) > 5:
        raise SystemExit(f"proportion analysis failed: {facing} jump shifts horizontally")
    if jump_max_h < walk_median_h * 0.82:
        raise SystemExit(f"proportion analysis failed: {facing} jump frames render too small against walk scale")
    if jump_min_h < walk_median_h * 0.65:
        raise SystemExit(f"proportion analysis failed: {facing} smallest jump frame collapses too much")
print(f"summary green_pixels={all_green}")
PY

echo "Proportion analysis captures written to $out_dir"
