# Pilgrim of the Thorn

A proof-of-concept Catholic gothic 2D platformer written in C. It uses OpenGL/GLUT so it can build on the local Mac without SDL or raylib.

## Build and Run

```sh
make
make run
make tune
./build/pilgrim --version
```

The game uses semantic versioning from the root `VERSION` file. Changing that file rebuilds the C executable with the new version and carries the same version into the macOS app, DMG, zip, and website.

## Package for macOS

```sh
make package-macos
```

This creates versioned artifacts such as `dist/macos/Pilgrim-of-the-Thorn-macOS-v0.3.0.dmg` and `dist/macos/Pilgrim-of-the-Thorn-macOS-app-v0.3.0.zip`. Open the DMG and drag `Pilgrim of the Thorn.app` onto the `Applications` shortcut. The app bundle includes the executable, a concept-art app icon, and all required `assets/` files.

## Deploy the Website and Current Game Build

From either the project root or `web/`:

```sh
npm run deploy
```

The root command delegates to the web project. Both entry points rebuild the C game, package the current semantic version, copy that versioned DMG into the website, build the Vite/Worker app, and deploy it with Wrangler.

To capture and sanity-check the main animation states:

```sh
make check-animations
make analyze-idle
make analyze-walk
make observe-walk
make review-walk
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

The game loads sprite placement from `assets/pilgrim_tuning.txt` on startup. In tuning mode, press `S` to save the current values back to that file, or set `PILGRIM_TUNE_FILE=/path/to/file.txt` to work against a different tuning file.

- `1` / `2` / `3` / `4` / `5`: edit idle, walk, jump, slash, or dash.
- `[` / `]`: select the previous or next frame in that animation.
- Arrow keys: nudge the selected frame. Hold Shift for 0.1-unit nudges or Control for 2-unit nudges.
- `T`: enter or leave trim mode for the selected frame.
- Trim mode arrow keys: trim the left, right, top, or bottom source edge. Hold Shift to restore outward beyond the original source box or Control for 4-pixel steps.
- `+` / `-`: scale width and height together.
- `Z` / `X`: shrink or grow width only.
- `C` / `V`: shrink or grow height only.
- `O`: reset the selected frame to the build's default tuning values.
- `S`: save all current tuning tables to the active tuning file.
- `L`: reload the active tuning file.
- `P`: print the current tuning tables to stderr and export C declarations to `/tmp/pilgrim_tuning_tables.c`, or to `PILGRIM_TUNE_EXPORT=/path/to/tables.c`.
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
- `assets/pilgrim_walk_cycle_source.png` is a dedicated 4-frame walk-cycle sheet: planted stride contact, rear foot lift, forward swing, descending pre-contact.
- Set `PILGRIM_CAPTURE=/tmp/frame.ppm make run` to dump the first rendered frame for visual checks.
- For animation checks, combine capture with `PILGRIM_SCRIPT=walk`, `PILGRIM_SCRIPT=jump`, `PILGRIM_SCRIPT=attack`, or `PILGRIM_SCRIPT=dash`, plus `PILGRIM_CAPTURE_FRAME=90` to capture after simulated input has advanced.
- Set `PILGRIM_TELEMETRY=/tmp/pilgrim.tsv` with a scripted run to log frame-by-frame position, velocity, grounded state, air time, and camera position.
- `make check-animations` captures idle, a four-frame walk-cycle sample, jump, slash, and dash frames into `/tmp/pilgrim_animation_check` and checks for obvious unkeyed chroma-green sprite leaks.
- `make analyze-idle` captures isolated idle breathing frames and checks planted-foot baseline/body-center drift.
- `make analyze-walk` captures isolated walk frames and checks planted baseline, body-center drift, and raised-step articulation.
- `make observe-walk` captures a timed scripted walk strip into `/tmp/pilgrim_walk_observe` so the slower articulated walk cadence can be reviewed frame-by-frame.
- `make review-walk` captures the current walk and writes visual review artifacts into `/tmp/pilgrim_walk_review`: an animated loop, numbered contact sheet, onion-skin overlay, guide overlay, metrics table, and matching Richter reference artifacts when the downloaded Richter sheet is present.
- `make analyze-jump` captures isolated jump frames and checks drift, edge margin, and chroma-key leaks.
- `make analyze-proportions` captures isolated idle, walk, jump, slash, and dash states and checks cross-state scale, centerline drift, and chroma-key leaks.
- Set `PILGRIM_FORCE_IDLE_FRAME=0..5` to capture a specific authored breathing panel.
- Set `PILGRIM_FORCE_WALK_FRAME=0..3` with `PILGRIM_SCRIPT=walk` to capture a specific walk-cycle panel for frame-by-frame review.
- Set `RICHTER_SHEET=/path/to/sheet.png` and optionally edit the `--reference-crop x,y,w,h` in `scripts/review_walk.sh` to compare against a different reference walk row.
- Set `PILGRIM_FORCE_JUMP_FRAME=0..3` to capture a specific jump-cycle panel for frame-by-frame review.
- Set `PILGRIM_FORCE_FACING=-1` or `1` to capture left- or right-facing animation frames.
