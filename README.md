# Pilgrim of the Thorn

A proof-of-concept Catholic gothic 2D platformer written in C. It uses OpenGL/GLUT so it can build on the local Mac without SDL or raylib.

## Build and Run

```sh
make
make run
make tune
```

To capture and sanity-check the main animation states:

```sh
make check-animations
make analyze-idle
make analyze-walk
make analyze-jump
make analyze-proportions
```

## Controls

- `A` / `D` or arrow keys: move
- `Space`, `W`, or up arrow: jump, hold briefly for a taller jump
- `J`: sword slash
- `K`: backdash
- `R`: reset to start
- `Esc`: quit

## Animation Tuning Mode

Run `make tune` or `PILGRIM_TUNE=1 make run` to play the game with the live sprite tuning overlay enabled. The current animation tables are edited in memory while you move, jump, dash, and attack, so placement changes can be judged against real gameplay motion. For a frozen isolated frame bench, run `PILGRIM_TUNE=1 PILGRIM_ISOLATE_PLAYER=1 ./build/pilgrim`.

- `1` / `2` / `3` / `4` / `5`: edit idle, walk, jump, slash, or dash.
- `[` / `]`: select the previous or next frame in that animation.
- Arrow keys: nudge the selected frame. Hold Shift for 0.1-unit nudges or Control for 2-unit nudges.
- `+` / `-`: scale width and height together.
- `Z` / `X`: shrink or grow width only.
- `C` / `V`: shrink or grow height only.
- `Q` / `E`: shrink or grow source-cell crop width for animations that support source cropping.
- `O`: reset the selected frame to the build's default tuning values.
- `P`: print the current tuning tables to stderr so tested values can be copied back into `src/main.c`.
- Optional capture helpers: set `PILGRIM_TUNE_ANIM=idle|walk|jump|slash|dash` and `PILGRIM_TUNE_FRAME=0..N` to open tuning mode on a specific frame.

## Prototype Notes

- The map spans several horizontal screens with a side-scrolling camera.
- Physics are tuned toward a Castlevania: Symphony of the Night-like platformer feel: quick ground acceleration, restrained air control, variable jump height, gravity-heavy falls, short backdash, light attack commitment, and smoothed camera follow.
- The visual direction is Catholic gothic: cloisters, cathedral arches, stained glass, candlelight, reliquary pickups, and a pilgrim-knight protagonist.
- `assets/cathedral_background.png` is a generated parallax background made for runtime use.
- `assets/midground_sheet_source.png` is a generated mid-parallax atlas with bell tower and ruined cloister elements.
- `assets/foreground_sheet_source.png` is a generated foreground atlas. The C loader keys out the green background at runtime.
- `assets/pilgrim_sheet_expanded_source.png` is the fallback/generated action sheet for jump, fall, slash, dash, and older idle/walk panels.
- `assets/pilgrim_idle_breath_source.png` is a dedicated 6-frame authored breathing cycle with planted feet.
- `assets/pilgrim_walk_cycle_source.png` is a dedicated 10-frame walk-cycle sheet used for smoother left/right leg movement.
- Set `PILGRIM_CAPTURE=/tmp/frame.ppm make run` to dump the first rendered frame for visual checks.
- For animation checks, combine capture with `PILGRIM_SCRIPT=walk`, `PILGRIM_SCRIPT=jump`, `PILGRIM_SCRIPT=attack`, or `PILGRIM_SCRIPT=dash`, plus `PILGRIM_CAPTURE_FRAME=90` to capture after simulated input has advanced.
- Set `PILGRIM_TELEMETRY=/tmp/pilgrim.tsv` with a scripted run to log frame-by-frame position, velocity, grounded state, air time, and camera position.
- `make check-animations` captures idle, a ten-frame walk-cycle sample, jump, slash, and dash frames into `/tmp/pilgrim_animation_check` and checks for obvious unkeyed chroma-green sprite leaks.
- `make analyze-idle` captures isolated idle breathing frames and checks planted-foot baseline/body-center drift.
- `make analyze-walk` captures isolated walk frames and checks baseline/body-center drift.
- `make analyze-jump` captures isolated jump frames and checks drift, edge margin, and chroma-key leaks.
- `make analyze-proportions` captures isolated idle, walk, jump, slash, and dash states and checks cross-state scale, centerline drift, and chroma-key leaks.
- Set `PILGRIM_FORCE_IDLE_FRAME=0..5` to capture a specific authored breathing panel.
- Set `PILGRIM_FORCE_WALK_FRAME=0..9` with `PILGRIM_SCRIPT=walk` to capture a specific walk-cycle panel for frame-by-frame review.
- Set `PILGRIM_FORCE_JUMP_FRAME=0..3` to capture a specific jump-cycle panel for frame-by-frame review.
- Set `PILGRIM_FORCE_FACING=-1` or `1` to capture left- or right-facing animation frames.
