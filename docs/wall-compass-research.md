# Wall tones and compass findings

Research and implementation date: 22 September 2026. Amethyst chose the PR probes and four
spoken directions by default, with a Text to Speech submenu and an optional eight-direction
setting. Answers are preserved in [wall-compass-questions.md](../../wall-compass-questions.md).
The implementation now includes all four wall recordings and the spoken compass; see
[todo.md](../../todo.md) for the installed build and pending listening checks.

## What PR 5435 does

Inspected PR head `257c6c0dfdf9cf9330735b43bc7512e1026d6fae` from
[PR 5435](https://github.com/HarbourMasters/Shipwright/pull/5435).
Preserved source is in the local build research folder, `spatial-audio-20260921/pr-source`.

`accessibility_cues.cpp` defines three `TerrainCueDirection` objects with relative yaw
0, +16384 and -16384. `scan()` combines these with `player->actor.world.rot`, so the probes
follow Link's facing, not the camera. They follow floor geometry for up to 500 game units,
stepping by 5.5 units. The complete scanner distinguishes walls, slopes, ledges, platforms,
water, lava, spikes and ground. It is not equivalent to three straight eye-height rays.

Wall checks use native collision (`BgCheck_EntitySphVsWall3`) and a player wall-line test for
height. The scanner handles changes of floor height and climbable steps; a wall result must
not be substituted for every unassigned terrain result. Cutscenes suppress scanning, and
`discoverWall()` suppresses wall cues in first-person mode. Other terrain behaviors and
their recordings remain separately unassigned.

The `Wall` class, lines 208–239, uses `NA_SE_IT_SWORD_CHARGE`, starts a sound when discovered,
then restarts it every 20 updates and seeks to PCM frame 88000. Its left and right paths
apply different pitch sweeps. It has no four-direction recording bank and does not label
walls north/east/south/west. The straight-ahead path leaves its local pitch modifier at
zero; this is an observed source value, not a verified claim about what that PR sounds like.
Do not blindly carry this pitch logic into the replacement recordings.

`ActorAccessibility_GeneralHelper`, `ActorAccessibility.cpp:532`, also reports actual wall
contact while Link attempts to move. It chooses three different game sounds from movement
distance squared: under 0.125, under 9, or otherwise. These are separate from the distant
wall probes and still have unanswered sound assignments.

The same helper's compass-like bell, lines 548–575, follows **Link's yaw**, not camera
direction. It runs when Link turns while stationary, or while L plus D-pad Down is held.
It varies a ship bell's pitch, pan and interval using yaw. It does not speak direction names
or consult the map. No new input binding or bell recording has been approved for this port.

## Time Stranger reference

Read these real files under `H:/projects/digimon story time stranger/mod/DSTS.Accessibility`:

- `Handlers/FieldAnnouncements.cs`: samples during operated field gameplay, establishes a
  silent first heading, and queues a direction when it changes. Invalid camera state clears
  the baseline. A map change also resets it.
- `Core/Compass.cs`: eight directions, `atan2(dx, -dz)`, with generated per-map north offsets.
  Its comments explicitly leave the offset sign unverified on nonzero maps. Ocarina must
  use its own verified camera vector and map axes, not those assumptions or memory offsets.
- `Core/WallSonar.cs`: 13 navmesh-boundary rays spaced 15 degrees across a forward half-circle,
  scanned every tenth call, within ten Time Stranger world units. Retains the nearest hit
  per cardinal direction; gain is `(1 - distance/range)^2`.
- `Handlers/FieldAudioHandler.cs:343`: supplies the listener yaw, then places the four wall
  sounds at hit X/Z and the player's height. `Core/SpatialAudioEngine.cs` uses looping wall
  voices in slots 20–23. The Ocarina requirement is a separate toggle and volume for each
  recording, unlike Time Stranger's single wall-sonar control.

There is a source-level discrepancy in the reference: WallSonar computes player-minus-hit,
so a +X wall selects its west slot, whereas Compass.Direction(+X, 0) returns east on a
zero-offset map. The north/south branches agree. WallSonar also does not apply the compass's
per-map offsets. This is an arithmetic comparison of those consumers, not an in-game test
or a change to Time Stranger. Share one verified bearing conversion for Ocarina speech and
wall selection so that they cannot diverge this way.

## Ocarina's actual direction reference

The extracted game resource `textures/icon_item_field_static/gWorldMapImageTex` contains a
compass rose with north at the top. It was decoded from the user's existing `game/oot.o2r`
using its real CI8 indices and RGBA16 palette, then visually inspected. The resource format
was verified against libultraship's `TextureFactory.cpp`; dimensions are 216 by 128.
The world-map renderer is `z_kaleido_map_PAL.c`.

`z_map_exp.c:Map_Init` supplies the map's scale/offset values. `Minimap_DrawCompassIcons`
places world X to the right and negative world Z upward, using positive scales from
`z_map_data.c`. Room offsets translate that mapping; they do not rotate it. Normal minimap
drawing covers 20 outdoor scenes and ten mapped dungeons. Other scenes can leave map
registers inherited from a previous scene, so those registers alone cannot prove support.
Dungeon map-up is a stable local navigation reference; it is not proof that every dungeon
interior or separately loaded boss arena aligns geographically with Hyrule Field.

The original English text resources independently name geographic directions:

- Message 0x5063: Death Mountain's trail is at the north end of Kakariko Village.
- Message 0x5067: the graveyard is east.
- Message 0x206f: Lake Hylia is south; Gerudo Valley is west.
- Message 0x7017: Lon Lon Ranch is south of town, across the field.
- Messages 0x4011 and 0x7009 also identify southern Lake Hylia and the western desert.

These are paraphrases of the native messages, read with the actual TextFactory binary
layout. Actual exit collision polygons provide a second, independent check. The scene
command, collision, surface and entrance layouts were followed through their resource
factories. `SurfaceType_GetSceneExitIndex` extracts data word zero, bits 8–12; the serialized
surface stores word one before word zero. Exit index zero means no exit; other indices are
one-based into the scene's exit list.

The averaged collision vertices at the relevant exits are:

- Kakariko to Death Mountain Trail: X -349.7, Y 560.8, Z -2050.0. North is negative Z.
- Kakariko to Graveyard: X 2261.0, Y 192.7, Z 1346.7. East is positive X.
- Hyrule Field to Lake Hylia: X -5874.7, Y -946.7, Z 16026.3. South is positive Z.
- Hyrule Field to Gerudo Valley: X -9959.8, Y -82.7, Z 6684.8. West is negative X.
- Field to Market entrance: X 20.0, Y 0.3, Z 230.0; to Ranch: X -2179.6, Y 293.8,
  Z 6117.5. The ranch lies south of the town exit, consistent with the dialogue.

These values are research evidence from loaded asset data, not runtime hardcoded anchors.
The executable feature should consume live camera, scene and collision structures.

`z_camera.c:Camera_Update` constructs view eye and target and passes them through
`func_800AA358`; `z_view.c` stores them as `view.eye` and `view.lookAt`, which its `guLookAt`
consumer uses to draw the world. Thus camera forward is lookAt minus eye. For an ordinary
unmirrored mapped scene, its horizontal bearing is `atan2(forward.x, -forward.z)`: 0 north,
90 east, 180 south, 270 west. A vertical/zero/nonfinite vector has no verified horizontal
heading and must be suppressed. Link's actor yaw is not a camera substitute.

Mirrored World flips the map's X coordinate and arrow rotation, and also flips the map
texture. The implementation reflects X in the shared bearing conversion, so both speech
and wall identity follow that displayed map. A reflected map's east is different from
the original geography's east; help text identifies this as map direction. Rooms without
supported maps and separately loaded boss arenas stay silent. Randomized entrance links
do not redefine a room's local map axes. Scene replacements have not been verified.
Support never comes from stale `gSaveContext.mapIndex` or map registers.

## Prepared assets and checks

All four original `research/wall_*.ogg` files in Time Stranger were converted without
normalization or pitch changes to mono PCM16, 32000 Hz. Source hashes were checked before
and after conversion. The four output files have distinct hashes, valid RIFF/PCM layouts,
and nonzero samples. Durations are north 3.120 seconds, east 3.080, south 4.645, west 2.280.
Conversion metadata is in `dependencies/accessibility-audio/manifest.json`; the packaging
list includes these approved assets. Each recording now has a toggle, volume and reset
action under Accessibility, Audio, initially On at 10 percent.

Local reproducible research tools are `inspect_compass_map.py`, `check_compass_landmarks.py`
and `prepare_wall_recordings.py` under
`C:/Users/Amethyst/source/ocarina-build/spatial-audio-20260921`. The first and third intentionally
refuse to overwrite their outputs. The map decoder needs Python 3.12 with the existing PIL
installation; the default Python 3.14 does not have PIL. No game launch was involved.

## Implemented behavior and corrections

`WallProbes.cpp` uses three temporary Link-relative probes and the native floor, sphere/wall,
ceiling and wall-line queries. Range remains 500, normal step size 5.5, and crawling keeps
the PR's smaller collision radius/height. Water and climbing have their own detection paths.
Reachable ledges, climbable surfaces, traversable slopes, spikes, lava, drops and water do
not acquire an unassigned wall recording. Sound output is limited to the 30 mapped scenes
listed in `WorldCompass.cpp`: 20 outdoors and ten dungeons. First-person aiming suppresses
wall tones as in the PR; camera-heading speech remains available while looking around.

The source trace exposed defects that are corrected rather than imported:

- The PR decreases its scan budget by `abs(vx + vz)`. At some diagonals those components
  cancel, and its vertical term subtracts a position from itself. The port bounds actual
  distance from Link and every probe movement, including climbing/water trials, to 512 steps.
- Its tall-wall branch continues before the final wall-discovery code, making that final
  branch unreachable. Tall blocking walls now produce a result.
- Its climbing branch writes to Link's actual rotation, and debug tracking can move Link.
  Temporary probe positions/rotations replace those writes; no fake player or shared
  player wall-check globals are mutated.
- Its climb flags sometimes compare the complete player state word for equality. The port
  tests the named bit, including when other player flags are present.
- Native wall, spike and climb properties receive the actual returned collision owner ID,
  including dynamic actors. Static geometry IDs are not substituted for moving walls.
- Climb-step wall tests pass the destination and origin in the order consumed by
  `BgCheck_EntitySphVsWall3`. Wall-height line tests face the probe being tested rather than
  using Link's unrelated facing for a side probe.

`SurfaceType_GetSlope` supplies the slip-surface property in this release; its consumer is
`Player_HandleSlopes`. The PR's floor-type checks use release accessor `func_80041D4C`, which
the native player stores as `sFloorType`; values 2 and 3 feed `func_80838144`'s hazard handling.
Climb flags use `func_80041DB8`. The native consumer `func_8083EC18` accepts bit 8
(vines), bit 2 (ladder ascent), and bit 4 through `func_80041E4C` (ladder descent).
The wall-sound exclusion now accepts all three. These are source-level APIs, not
hardcoded runtime offsets.

`CueActors.cpp` selects a wall recording from wall-minus-Link position with `MapDirection`.
Each probe retains its own voice. The recording plays in full and repeats while detected,
instead of imposing the PR's original-sample seek offset or cutting the replacement after
20 updates. Moving a source updates its position without restarting the recording. Changing
cardinal identity changes recording; disappearance, disabling, pause or scene teardown stops
it. An unavailable replacement stops the old voice rather than playing the wrong direction.
PR distance falloff, Link distance, camera position/direction, selected output mode and the
existing native reverb send all remain in the shared cue mixer.
Native `Player_SetupTalk` sets both `PLAYER_STATE1_TALKING` and `PLAYER_STATE1_IN_CUTSCENE`,
so the existing cue cutscene guard also stops wall voices during ordinary conversations.

`CompassSpeech.cpp` samples the actual view's horizontal forward vector once per gameplay
frame. Its first sample establishes a silent baseline. It coalesces a turn until three
samples are steady, then queues only a changed direction through `TTSSpeakLocalized` and
the existing Prism bridge. Menus, dialogue, native cutscene/transition states, invalid camera
data, and unsupported scenes clear the baseline. It does not introduce a compass binding
or the PR's orientation bell. English, French and German direction words live in the central
misc speech banks.

Accessibility, Text to Speech contains Enable speech, Spoken compass, Compass directions
(four/eight) and Reset speech settings. The existing `gSettings.A11yTTS` value is preserved.
The new keys are `gSettings.A11yTTSCompass` (default 1) and
`gSettings.A11yTTSCompassDirections` (default 4). Wall keys follow the existing
`gSettings.A11yAudio.wall_north/east/south/west.Enabled/Volume` convention. Native CVar save
and change handling is retained. Inspection found no generic reset action on native cue
rows, so each of all 13 sounds now has an explicit reset restoring On and 10 percent.

Offline checks compile the production scanner against controlled native-query fixtures.
They cover probe rotation, every sampled diagonal, maximum work/range, ordinary/tall walls,
reachable ledges, slopes, voids, water, climbing flags, spike/lava exclusions, dynamic owner
IDs, and byte-for-byte preservation of Link's state. Bearings cover all cardinals, optional
diagonals, reflection, wrapping, unsupported scenes, nonfinite input, settled turns and
boundary jitter. The real cue mixer tests all 13 recordings, continued looping without
per-frame restarts, direction changes, gain and cleanup. These four audio/navigation suites,
two existing native Options suites and the existing speech suite pass. The fixtures do not
replace listening tests in real rooms or a performance measurement in the running game.

## Follow-up: landing floors and ladder descent, 23 September

Amethyst asked whether the pathfinder's rejected floor contacts also require changing
wall sounds. Both features use `BgCheck_EntitySphVsWall3`, but with different movement.
`BgCheck_CheckWallImpl` includes floor polygons in its downward line sweep when
`checkHeight + dy < 5`. That result caused the pathfinder's former jump-landing failure.
The ordinary wall probe follows the floor, stops at drops of 20 or more, and excludes
smaller descending slopes before testing walls. At normal height 26, a descent large
enough to trigger that sweep has already stopped the probe; the crawling height 15
also has its relevant descents excluded. The ordinary probe's 5.5-unit steps are
smaller than its minimum radius of 10, so the other long-horizontal-sweep floor branch
is not used there either. Special climbing and swimming trials are separate paths;
these findings do not establish every terrain case in those paths.

The production wall scanner was linked into `scene_navigation_checks` with the real
`z_bgcheck.c` and the private Kokiri collision resource. Before and after the flag fix,
it correctly leaves the upper-platform edge at (-519, 200, -1030), facing negative X,
and the porch ladder approach at (-31, 100, 1073), facing negative Z, silent. These
checks also verify floor support and byte-for-byte preservation of Link's state.
Sliding-terrain callbacks deliberately abort in this fixture rather than simulate
untested player physics. No live actors or controller input are simulated.

A separate controlled-query regression failed before the fix: a returned ladder
descent surface with flags 5 was classified as an ordinary wall. The native climbing
consumer accepts its bit 4 and starts the downward entry animation. The omission is
now corrected in `Probe::ClimbableWall`; the floor query itself is unchanged. The
checked porch approach was already silent, so this is not a reproduced false porch
tone. The scanner suite now checks ascent (3), descent (5) and vines (8), alongside
existing ordinary walls, ledges, slopes, hazards and dynamic collision ownership.

The scanner suite and both native-scene wall checks pass. Logs are under
`C:/Users/Amethyst/source/ocarina-build/wall-filter-20260923`. Listening and the earlier
pathfinder controller tests remain pending in `../../todo.md`.
