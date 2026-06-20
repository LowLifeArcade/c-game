# Pilgrim of the Thorn

A proof-of-concept Catholic gothic 2D platformer written in C. It uses OpenGL/GLUT so it can build on the local Mac without SDL or raylib.

## Build and Run

```sh
make
make run
```

To capture and sanity-check the main animation states:

```sh
make check-animations
```

## Controls

- `A` / `D` or arrow keys: move
- `Space`, `W`, or up arrow: jump, hold briefly for a taller jump
- `J`: sword slash
- `K`: backdash
- `R`: reset to start
- `Esc`: quit

## Prototype Notes

- The map spans several horizontal screens with a side-scrolling camera.
- Physics are tuned toward a Castlevania: Symphony of the Night-like platformer feel: quick ground acceleration, restrained air control, variable jump height, gravity-heavy falls, short backdash, light attack commitment, and smoothed camera follow.
- The visual direction is Catholic gothic: cloisters, cathedral arches, stained glass, candlelight, reliquary pickups, and a pilgrim-knight protagonist.
- `assets/cathedral_background.png` is a generated parallax background made for runtime use.
- `assets/midground_sheet_source.png` is a generated mid-parallax atlas with bell tower and ruined cloister elements.
- `assets/foreground_sheet_source.png` is a generated foreground atlas. The C loader keys out the green background at runtime.
- `assets/pilgrim_sheet_expanded_source.png` is a generated character sprite sheet. The C animation system swaps source rectangles for 4-frame idle, 6-frame walk, 4-frame jump/fall, and 4-frame sword slash states.
- `assets/pilgrim_walk_cycle_source.png` is a dedicated 10-frame walk-cycle sheet used for smoother left/right leg movement.
- Set `PILGRIM_CAPTURE=/tmp/frame.ppm make run` to dump the first rendered frame for visual checks.
- For animation checks, combine capture with `PILGRIM_SCRIPT=walk`, `PILGRIM_SCRIPT=jump`, `PILGRIM_SCRIPT=attack`, or `PILGRIM_SCRIPT=dash`, plus `PILGRIM_CAPTURE_FRAME=90` to capture after simulated input has advanced.
- Set `PILGRIM_TELEMETRY=/tmp/pilgrim.tsv` with a scripted run to log frame-by-frame position, velocity, grounded state, air time, and camera position.
- `make check-animations` captures idle, a ten-frame walk-cycle sample, jump, slash, and dash frames into `/tmp/pilgrim_animation_check` and checks for obvious unkeyed chroma-green sprite leaks.
- Set `PILGRIM_FORCE_WALK_FRAME=0..9` with `PILGRIM_SCRIPT=walk` to capture a specific walk-cycle panel for frame-by-frame review.
