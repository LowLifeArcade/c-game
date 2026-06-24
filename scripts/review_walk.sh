#!/usr/bin/env bash
set -euo pipefail

out_dir="${PILGRIM_WALK_REVIEW_DIR:-/tmp/pilgrim_walk_review}"
capture_dir="${PILGRIM_WALK_REVIEW_CAPTURE_DIR:-/tmp/pilgrim_walk_review_captures}"
richter_sheet="${RICHTER_SHEET:-/Users/Sonny/Downloads/PlayStation - Castlevania_ Symphony of the Night - Playable Characters - Richter Belmont.png}"
walk_frames="${PILGRIM_WALK_FRAMES:-$(python3 scripts/animation_clip_info.py walk frames)}"

mkdir -p "$out_dir" "$capture_dir"

for ((frame = 0; frame < walk_frames; frame++)); do
  ppm="$capture_dir/right_walk_$frame.ppm"
  rm -f "$ppm"
  PILGRIM_ISOLATE_PLAYER=1 \
  PILGRIM_SCRIPT=walk \
  PILGRIM_FORCE_FACING=1 \
  PILGRIM_FORCE_WALK_FRAME="$frame" \
  PILGRIM_CAPTURE="$ppm" \
  PILGRIM_CAPTURE_FRAME=95 \
  ./build/pilgrim
done

python_bin="${PILGRIM_REVIEW_PYTHON:-}"
if [[ -z "$python_bin" ]]; then
  bundled="$HOME/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/bin/python3"
  if [[ -x "$bundled" ]]; then
    python_bin="$bundled"
  else
    python_bin="python3"
  fi
fi

"$python_bin" scripts/review_walk.py \
  --frames-dir "$capture_dir" \
  --out-dir "$out_dir" \
  --walk-frames "$walk_frames" \
  --reference-sheet "$richter_sheet" \
  --reference-crop "0,135,214,60"

echo "Walk review artifacts written to $out_dir"
