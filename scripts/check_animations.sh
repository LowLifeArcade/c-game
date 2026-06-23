#!/usr/bin/env bash
set -euo pipefail

out_dir="${PILGRIM_CHECK_DIR:-/tmp/pilgrim_animation_check}"
mkdir -p "$out_dir"

capture() {
  local name="$1"
  local script="$2"
  local frame="$3"
  local ppm="$out_dir/$name.ppm"
  local png="$out_dir/$name.png"
  rm -f "$ppm" "$png"
  if [[ "$script" == "idle" ]]; then
    PILGRIM_ISOLATE_PLAYER=1 PILGRIM_CAPTURE="$ppm" PILGRIM_CAPTURE_FRAME="$frame" ./build/pilgrim
  else
    PILGRIM_ISOLATE_PLAYER=1 PILGRIM_SCRIPT="$script" PILGRIM_CAPTURE="$ppm" PILGRIM_CAPTURE_FRAME="$frame" ./build/pilgrim
  fi
  sips -s format png "$ppm" --out "$png" >/dev/null 2>&1 || true
  python3 - "$ppm" "$name" <<'PY'
import sys
path, name = sys.argv[1], sys.argv[2]
with open(path, "rb") as f:
    header = f.readline()
    if header.strip() != b"P6":
        raise SystemExit(f"{name}: unsupported capture format")
    dims = f.readline()
    while dims.startswith(b"#"):
        dims = f.readline()
    w, h = map(int, dims.split())
    maxv = int(f.readline())
    data = f.read()
green = 0
for i in range(0, len(data), 3):
    r, g, b = data[i], data[i + 1], data[i + 2]
    if g > 180 and g > r + 80 and g > b + 80:
        green += 1
print(f"{name}: {w}x{h}, chroma-green pixels={green}")
if green > 0:
    raise SystemExit(f"{name}: possible unkeyed sprite-sheet leak")
PY
  echo "$png"
}

capture idle_a idle 5
capture idle_b idle 52
for idle_frame in 0 1 2 3 4 5; do
  PILGRIM_FORCE_IDLE_FRAME="$idle_frame" capture "idle_breath_$idle_frame" idle 10
  PILGRIM_FORCE_FACING=-1 PILGRIM_FORCE_IDLE_FRAME="$idle_frame" capture "left_idle_breath_$idle_frame" idle 10
done
capture walk walk 95
for walk_frame in 0 1 2 3; do
  PILGRIM_FORCE_WALK_FRAME="$walk_frame" capture "walk_cycle_$walk_frame" walk 95
  PILGRIM_FORCE_FACING=-1 PILGRIM_FORCE_WALK_FRAME="$walk_frame" capture "left_walk_cycle_$walk_frame" walk 95
done
capture jump_takeoff jump 18
capture jump_air jump 38
for jump_frame in 0 1 2 3; do
  PILGRIM_FORCE_JUMP_FRAME="$jump_frame" capture "jump_cycle_$jump_frame" idle 10
  PILGRIM_FORCE_FACING=-1 PILGRIM_FORCE_JUMP_FRAME="$jump_frame" capture "left_jump_cycle_$jump_frame" idle 10
done
capture slash attack 28
PILGRIM_FORCE_FACING=-1 capture left_slash attack 28
capture dash dash 34
PILGRIM_FORCE_FACING=-1 capture left_dash dash 34

echo "Animation captures written to $out_dir"
