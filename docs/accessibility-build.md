# Accessibility fork: build and installation

## Source

- Fork: https://github.com/RealAmethyst/Shipwright
- Upstream: https://github.com/HarbourMasters/Shipwright
- Starting release: Ackbar Delta 9.2.3, published 14 April 2026.
- Release commit: `cb71e22a79bc5d1f688fa881795bbd93094895fc`.
- Local branch: `accessibility`, created directly at that tag.
- Checkout: `H:\projects\ocarina\Shipwright`.

GitHub verified RealAmethyst/Shipwright as a fork of HarbourMasters/Shipwright on 21 September 2026. Origin points to the fork and upstream to HarbourMasters. Accessibility development uses the `accessibility` branch. The changes restore native desktop setup dialogs and integrate Prism speech for extraction, boot logos and the existing game TTS, including menu positions and announcement ordering. Wider gameplay accessibility remains future work.

## Current pathfinder speed build

Built and deployed on 23 September 2026 as `5504dc3ec85d`, at
`H:/projects/ocarina/game/soh.exe`. Native collision topology, lazy candidate validation
and a 6 ms search slice reduce search time. The route follows Link's current position,
retains its target through temporary loss of support and rechecks the original climb
approach. [Source findings and benchmarks](pathfinder.md) record the verified causes,
five-route comparison and remaining movement limits. The Documents extraction-test
copy remains on `3070a31e1701`.

Amethyst subsequently tested this build, reported that the pathfinder is "now much
better," and authorized committing and pushing the completed work. Individual
route, listening and broader map checks remain in the root checklist.

- Executable SHA-256: `5504dc3ec85dd8c9dc385cad5c19cf0dc37e320facb2328312cbc59181ca58f2`.
- Symbols SHA-256: `7887d4207ca315475338890597d395edfafc1bd7c6f68efff943237cc36bd500`.
- Port archive SHA-256: `ec56086b2da94c03a1582394d0958b3ac43742719172889a8bdc1a8168d8cc49` (unchanged).
- ZIP: `_packages/Ship-9.2.3-pathfinder-speed-final-prism-win64-ship.zip` (102,193,132 bytes).
- ZIP SHA-256: `65bfe3fa61d114136f329e18e72264e8dad9867ab695c301ba003d73d8840446`.

Release build and all 16 navigation CTest entries passed. Coverage includes native
Kokiri movement replay, ladder/ledge revalidation, corner following, contact origins,
actual player OC dimensions, reconnection after movement and native walking surveys
in Hyrule Field, Kakariko and the Deku Tree. The median old/new search update counts
were 61/4 (porch heart), 103/11 (porch crawlspace), 10/2 (porch shop), 97/9 (ground
crawlspace), and 282/3 (porch Lost Woods), across three runs per case. These are
offline scheduler counts; game frame timing, live actors and controller steering
still need Amethyst's test. Unchanged mixer, Options and speech suites retain their
previous results.

All 7,961 package entries and 1,281
port entries passed CRC validation. Source resources, x64 GUI executable, pinned
runtime dependencies and thirteen recordings passed verification. No ROM-derived
fixtures, archives, saves, settings or logs were packaged. Resources did not change.

Pre-task build `3849f4cbee59`, symbols, port archive, checklist and log are preserved
in `H:/projects/ocarina/backups/9.2.3-before-pathfinder-speed-20260923`. Protected
archives, saves, settings, layouts, runtime libraries and recordings were verified
unchanged. Intermediate build `9150cdfec9f2` is separately preserved in
`../../backups/9.2.3-before-pathfinder-speed-final-20260923`. That intermediate
deployment preceded removal of a repair-search cancellation condition: joining an
old waypoint does not prove the blocked next corridor has reopened. Root records
and the preserved benchmark source are in
`../../backups/pathfinder-speed-records-20260923`.

Build, test, benchmark, packaging and `pathfinder-speed-final-deployment.json` records are
under `C:/Users/Amethyst/source/ocarina-build/pathfinder-speed-20260923`. Navigation
tests additionally configure `NAV_EXTRA_SCENES` to that private fixture directory.
No game was launched or controlled, and no commits, pushes or submodule changes
were made. Launch the game afresh; no extraction, settings reset or Windows restart
is needed. Current manual checks are in `../../todo.md`.

## Previous wall-filter build

Built and deployed on 23 September 2026 as `3849f4cbee59`, at
`H:/projects/ocarina/game/soh.exe`. Wall sounds now exclude native ladder descent
surfaces, matching the player's actual climb-entry flags. The preceding pathfinder
movement fixes are included. The normal wall scan already avoided the tested Kokiri
landing-floor case; [the source findings](wall-compass-research.md) distinguish that
from the separate flag omission. At that handoff, the preceding movement update still awaited Amethyst's test; the later speed/guidance report is addressed above.
The Documents extraction-test copy remains on `3070a31e1701`.

- Executable SHA-256: `3849f4cbee59d590e4f87e7fd234ba7cf2a49da2473707dceca53eac4336998f`.
- Symbols SHA-256: `992a4c575be56e8403a0c94e1bb4fe63d1b73e254fd5b4a0dd91a41ea8c20f6d`.
- Port archive SHA-256: `ec56086b2da94c03a1582394d0958b3ac43742719172889a8bdc1a8168d8cc49` (unchanged).
- ZIP: `_packages/Ship-9.2.3-wall-filter-prism-win64-ship.zip` (101,789,976 bytes).
- ZIP SHA-256: `a4213d6d9f326b68bdc2fb98b3b1cf5ad354f20111aa71dcd458d51862e8f728`.

Release build, `wall_compass_checks`, `scene_wall_floor_drop` and
`scene_wall_porch_ladder` passed. The controlled ladder-descent test failed before
the fix; both native Kokiri wall cases passed before and after. Other navigation,
mixer, Options and speech code is unchanged and retains its recorded test results.
Package and port CRCs, source resources, pinned runtimes and all thirteen recordings
passed verification. No ROM-derived assets, saves, configuration or logs were packaged.

Previous build `b0458fd19e51`, symbols, port archive, checklist and log are preserved in
`H:/projects/ocarina/backups/9.2.3-before-wall-filter-20260923`. Protected game archives,
saves, settings, layouts, runtime libraries and recordings were verified unchanged.
Root notes before editing are in `../../backups/wall-filter-records-20260923`.
Logs and `wall-filter-deployment.json` are under
`C:/Users/Amethyst/source/ocarina-build/wall-filter-20260923`.

No game was launched or controlled; there were no commits, pushes or submodule changes.
Launch the normal game afresh. No extraction, settings reset or Windows restart is
needed. The listening and controller checklist remains in `../../todo.md`.

## Previous pathfinder movement build

Built and deployed on 23 September 2026 as `b0458fd19e51`, at
`H:/projects/ocarina/game/soh.exe`. This update connects native ladders and climbable-wall
ascent, corrects sloping ledge approaches/landings and native downward floor-sweep
contacts, preserves guidance through traversal animations, and uses native pickup bounds.
The three logged porch failures now pass in the real collision fixture. Previous menu,
distance, entrance naming/grouping and verified gate filtering remain included.
The separate Documents extraction-test copy remains on `3070a31e1701`.

- Executable SHA-256: `b0458fd19e5177ecc26f0ffacdc323b5614a0fa385c26e4cae9969511607b8ea`.
- Symbols SHA-256: `a3b77a76995a3c39f58329a12cff2223cac4cf6b785e0b386d9bdb52c763d9f8`.
- Port archive SHA-256: `ec56086b2da94c03a1582394d0958b3ac43742719172889a8bdc1a8168d8cc49`.
- ZIP: `_packages/Ship-9.2.3-pathfinder-flow-prism-win64-ship.zip` (101,669,388 bytes).
- ZIP SHA-256: `f6e3805f93148cccda9e39404657da0219ecdb74c0f101a2c18442cd0330e325`.

All 7,961 package entries and 1,281 port entries passed CRC checks.
Resources match source, all thirteen recordings and runtime dependencies match their
pinned files, and the executable is x64 GUI. No ROM, extracted game archive, collision
fixture, private map rendering, save, settings or logs were packaged. The manifest is
`C:/Users/Amethyst/source/ocarina-build/pathfinder-flow-20260923/pathfinder-flow-deployment.json`.
Previous build `c4dbf0d6521e`, symbols, port archive, checklist and log are preserved in
`H:/projects/ocarina/backups/9.2.3-before-pathfinder-flow-20260923`.
Deployment checked the game was closed and verified protected archives, saves, settings,
layouts, runtime libraries, controller database and recordings unchanged.

The Release build and all eleven navigation CTest entries passed. Coverage includes
the three exact logged failures (shop, crawlspace, heart), both directions on the porch
ladder, climbable-wall ascent, sloped ledges, the upper-area return, a native floor-sweep
landing and all nine static Kokiri entrance thresholds. Synthetic regressions cover
pickup bounds and native traversal-state sequences, plus existing wall/ceiling/hazard
safeguards. Removing climbable flags rejects the corresponding native-fixture links.
Every reconstructed scene traversal must survive runtime-style revalidation.

No resources changed, so the port archive is unchanged. Unchanged audio, Options and
speech suites were not rerun; previous manifests retain their results. The scene
survey does not instantiate live NPC/story actors. Controller steering, ladder controls,
climb timing, pickup arrival speech and listening still need Amethyst's test in `../../todo.md`.
The optional spoken climb-direction instruction remains pending in the root question file.

Logs in `C:/Users/Amethyst/source/ocarina-build/pathfinder-flow-20260923`:
`build-release-final.log`, `build-scene-checks-final.log`, `navigation-tests-final.log`, and `package.log`.
The same folder holds the private geometry image and before/after route evidence.
[Source findings](pathfinder.md) record native consumers, map coordinates and remaining limits.

The standalone navigation checks use the existing toolchain and dependencies:

```powershell
# The extractor creates a new file; it refuses to overwrite an existing fixture.
py -3.12 H:/projects/ocarina/Shipwright/tests/navigation/extract_scene_fixture.py H:/projects/ocarina/game/oot.o2r C:/Users/Amethyst/source/ocarina-build/navigation-field-checks/kokiri.collision
& $shipCmake -S H:/projects/ocarina/Shipwright/tests/navigation -B C:/Users/Amethyst/source/ocarina-build/navigation-checks -A x64 -DNAV_VCPKG:PATH=C:/Users/Amethyst/source/ocarina-build/vcpkg/installed/x64-windows-static -DNAV_KOKIRI_FIXTURE:FILEPATH=C:/Users/Amethyst/source/ocarina-build/navigation-field-checks/kokiri.collision
& $shipCmake --build C:/Users/Amethyst/source/ocarina-build/navigation-checks --config Release --parallel 4
& $shipCtest --test-dir C:/Users/Amethyst/source/ocarina-build/navigation-checks -C Release --output-on-failure
```

`shipCmake` and `shipCtest` refer to the bundled CMake and adjacent CTest under the
Visual Studio 2022 Build Tools path documented below. Never package the private fixture.
No libultraship changes, commits, pushes or game launches were made.
The pre-change checklist, release guide, questions, log and source/build notes are preserved
in `../../backups/pathfinder-flow-records-20260923`. Launch the normal game afresh;
no extraction, settings reset or Windows restart is required.

## Previous setup build, retained in the Documents test copy

Built and deployed on 22 September 2026. Build ID: `3070a31e1701`. Launch paths:
`C:\Users\Amethyst\Documents\ocarina\soh.exe` for the fresh-install test and
`H:\projects\ocarina\game\soh.exe` for ordinary play. This build moves graphics-debugger
initialization before window construction, fixing the null service used by setup rendering
after ROM selection. See [the crash evidence](setup-crash-20260922.md). Audio and Prism code
are unchanged. Headphone uses Steam Audio HRTF for verified world
sources; Surround uses Windows spatial sound with a 7.1.4 bed. Stereo and Mono retain the
native path. Music stays outside Steam Audio. A Windows headphone spatial provider can
still externalize the stereo music bed in Surround; see [spatial-audio.md](spatial-audio.md).

Thirteen recordings are bundled: item, person, door, transition, destructible, crawlspace,
ladder, elevator, pathfinder, and north/east/south/west wall tones. Their approved PR 5435
policies cover 73 actor kinds, collision-polygon exits/climbable surfaces, 65 distinct
crawlspace/route locations and three Link-relative wall probes. Cues share native room reverb.
Every recording has an On/Off switch, volume and reset under Accessibility, Audio, defaulting
to On and 10 percent. Unassigned meanings remain pending; see [the inventory](spatial-cue-inventory.md).

Accessibility, Text to Speech contains the existing speech switch, a spoken-compass switch,
four/eight direction choice (default four), and reset. Directions follow the camera and the
supported area's map, including reflection in mirrored play. Wall tones and compass are
limited to 20 outdoor maps and ten mapped dungeons; other interiors/boss arenas stay silent.
See [wall-compass-research.md](wall-compass-research.md) for source evidence and scanner corrections.
Ordinary cue repetition now follows Time Stranger: complete clip plus 60 ms, a roughly
0.4-second destructible excerpt, and 2 ms wall-loop overlap. Person repeats every 0.448 seconds.
Obsolete ordinary frame timers were removed; special-item approach pulses retain their
signals. See [cue-loop-timing.md](cue-loop-timing.md). The port archive, recordings and saved
settings are unchanged. The prior file-select return, menu feedback and Prism changes remain included.

Steam Audio is pinned to 4.8.1. CMake verifies the SDK ZIP hash in `CMake/SteamAudio.cmake`.
The SOFA HRTF is pinned in `dependencies/steam-audio`; provenance and SHA-256 values for
every recording are in `dependencies/accessibility-audio/manifest.json`. CMake/CPack installs
runtime assets and notices. No other game's files are needed at runtime.

SHA-256 values:

- Package `_packages/Ship-9.2.3-setup-fix-prism-win64-ship.zip`: `df9ac94537c78429e61a6d065dd01af119549f9217bfe9c4c1740a61c05a4c5d`.
- `soh.exe`: `3070a31e1701019b519266f2ad455657a188066c282e211f3de94ec6f28deaac`.
- `soh.o2r`: `22207ca2cbfcc8881e4825a46c710493f82aa3c6bd5cbfe21bca5ecb72993dce`.
- `phonon.dll`: `ca3dbc01dbc24492717011e80f6a51404ca143ae344ca660971d2c983f1e058d`.
- `prism.dll`: `cb9712e11af9ebe96457dbf8f5daad4a6c359ae1f59cdf2663282b3a9cc9759c`.
- `debug/soh.pdb`: `13e6b7b5ffda6b3e549b1b7c01553791c59906168490229e9d599a4d912cd858`.

All 7,961 package entries and 1,280 port entries passed CRC checks.
Packaged accessibility text matches source. The executable is Windows x64 GUI; packaged
runtime DLLs and all thirteen recordings match their verified source hashes. No ROMs,
extracted game archives, saves or personal configuration are packaged.

Deployed files match the package in both copies. Only their executable and symbols changed.
The prior binaries, checklist and installation guide are backed up in
`H:\projects\ocarina\backups\9.2.3-before-setup-fix-20260922`, preserving build `d4342490da07`.
The earlier `9.2.3-before-cue-loops-20260922` backup preserves build `05bfdd1a3a0e`.
Build `d1b8d8a98841` remains in `9.2.3-before-wall-compass-20260922`. The first spatial build `cb0b1718f56e` remains in the earlier
`9.2.3-before-spatial-cues-20260922` backup, and the pre-spatial build remains in
`9.2.3-before-spatial-audio-20260922`. Game archives, saves, configuration, Prism and overlay
layout were hash-verified unchanged. No game was launched, controlled or closed. Amethyst
retries extraction in the Documents copy herself; the normal installation needs no extraction.
No Windows restart is needed. Checks are in
[todo.md](../../todo.md).

The Release build, both Native Options suites and the speech suite passed for this setup fix.
The four audio/wall/compass suites passed for the preceding cue-timing update, including
reference timing and waveform checks for all thirteen loops; their code is unchanged here.
The production wall scanner uses controlled native-query
fixtures for offline verification; this does not prove real-room coverage or runtime cost.
All thirteen recordings passed playback/completion checks, with continued looping and
cleanup tested for walls. The earlier production Windows backend probe rendered 72,000
silent frames; this does not establish enabled Atmos or audible height speakers. The earlier
HRTF benchmark rendered 64 continuous sources for one audio second in about 61 milliseconds.
Windows x64 was built; Linux/macOS HRTF paths remain untested here.

Current build/package logs and `setup-fix-deployment-manifest.json` are in
`C:\Users\Amethyst\source\ocarina-build\setup-crash-20260922`:
`build-setup-fix.log` and `package-setup-fix.log`. The manifest records both target copies,
their previous executable/symbol hashes, backups and unchanged personal files. Complete
extraction remains untested in game. Use the setup-fix ZIP in place of the cue-loops ZIP.

Earlier audio logs and `cue-loops-deployment-manifest.json` are in
`C:\Users\Amethyst\source\ocarina-build\spatial-audio-20260921`. Final build log:
`build-cue-loops.log`; package log: `package-cue-loops.log`; test build log:
`build-cue-loops-checks.log`. The unchanged port archive was generated previously in
`generate-wall-compass-reset.log`. Earlier deployment records are retained. The spatial CMake test
project is `tests/spatial-audio`; configure `STEAM_AUDIO_SDK` to the extracted pinned SDK
and `SPATIAL_VCPKG` to the existing `x64-windows-static` triplet. Use the VS 2022 Release
commands below; regenerate `GenerateSohOtr` when text changes. The existing verified SDK was
reused with `FETCHCONTENT_SOURCE_DIR_STEAM_AUDIO`; fresh builds use the pinned download.

Amethyst's earlier publication request was completed with Shipwright `260e71384` and
libultraship `3f465487` on their `accessibility` branches. This subsequent setup fix and its
documentation remain local and uncommitted; the library is unchanged. The
`accessibility` branch in `RealAmethyst/libultraship` supplies the modified library;
`.gitmodules` points to that fork. Publish the library first, followed by the parent branch.
Prior local accessibility commits remain in both histories. The installation/release guide
is outside this repository at `../../installation-and-release.md`, as requested, and is not
part of either the source commit or the release ZIP. Publication does not create a GitHub
release or upload the binary package automatically.

## Previous native Options build

Built and deployed on 21 September 2026. Build ID: `66a8f25eff60`. Launch path: `H:\projects\ocarina\game\soh.exe`.
Returning from native Options to file selection now announces the live focused item and its position once, without
repeating the parent heading or hints. Both B/Backspace and Escape close paths restore speech. This also supports
Escape over another file row or a native confirmation. See [accessibility-speech.md](accessibility-speech.md) for
the cache/lifetime trace and regression coverage. No text or art assets changed.

Amethyst confirmed the previous menu feedback update, build `daf31abcf6b7`, works apart from this return announcement.
Prism speech, native menu sounds, live fullscreen values, closed startup and the graphics-buffer fix remain in place.

SHA-256 values:

- Package `_packages/Ship-9.2.3-native-options-file-return-prism-win64-ship.zip`: `70b00cac804abc45b8ff4afe2ef945b33dcafd0f2db9429984e70ba9f33f7836`.
- `soh.exe`: `66a8f25eff60834f3dd77df15125e8ec3f8b6b4d1cb0cf9b1d1b128161a72521`.
- `soh.o2r`: `69b6f8688a455ae32c84c5fc3da2c8f31f6a30e590d751bed769772d63decae4`.
- `prism.dll`: `cb9712e11af9ebe96457dbf8f5daad4a6c359ae1f59cdf2663282b3a9cc9759c`.
- `debug/soh.pdb`: `506a9cc931f98ab4dc1ba7112b9fbc2fda13e66614ee362b03754bc70be81524`.

All 7,937 package entries and 1,280 port-archive entries passed CRC checks.
Packaged speech banks match source. The executable is Windows x64 with the GUI subsystem; Prism remains the pinned
0.18.2 DLL. The package contains no ROM-derived game archive.

Only the installed executable and debug symbols were replaced, and match the package/build output. The previous
files, log and testing notes are preserved in `H:\projects\ocarina\backups\9.2.3-before-file-return-20260921`. Extracted game archives, port assets, Prism DLL,
configuration, overlay layout and saves were verified unchanged during deployment.

The Release build, speech suite and both native-options CTest suites passed. New checks cover unchanged focus on
return, delayed text, suppression of repeated headings/hints, another row or confirmation, native child returns,
real reopening and initial queue order. The existing display-list, feedback, editor and overlay checks still pass.

Logs and `file-return-manifest.json` are in `C:\Users\Amethyst\source\ocarina-build`:
`build-file-return.log`, `check-file-return-final.log`, `package-file-return.log`.
No game was launched or controlled by Codex. The focused file-return test remains in [todo.md](../../todo.md).
No extraction or Windows restart is needed.

## Earlier local accessibility checkpoint

At this earlier checkpoint, the Shipwright accessibility branch included the native setup, Prism and native Options work. Its libultraship
submodule pointed to local commit `0eeb24cbfb03f324e342ee95ff84087b765e2383` on that submodule's `accessibility` branch.
The source contents were built before recording the commits; the installed executable is identified by the hash
above, and its embedded release base still says cb71e22. Both commits include Codex attribution.

No commits had been pushed at that checkpoint. Those commits were subsequently included in
the 22 September publication described above. Game data, downloaded dependencies, logs,
build outputs and distribution archives are excluded from the commits.

## Previous Prism build

Built and deployed on 21 September 2026. Build ID: `0f25fa93765b`. The installed executable is `H:\projects\ocarina\game\soh.exe`. Speech defaults to enabled; the Text to Speech checkbox remains and the F9 toggle is removed.

Prism is pinned to release 0.18.2, source revision `f237af67d0460a6dad312cc446623856ad823c50`. The official Windows x64 dependency ZIP is verified against SHA-256 `31c02e3ef2260b4d3b11fb00132f8eb12bc147b5fb17031b580670b117ba7d23`. The package includes `prism.dll`, its licenses and attribution. See [the speech notes](accessibility-speech.md) for the verified API, source findings and platform limitations.

Previous Prism SHA-256 values:

- Package `_packages/Ship-9.2.3-prism-win64-ship.zip`: `a6e66ef22eae72c7614a7255c459e3ae57a6c4eb6e2369df25a8dccf2b466e76`.
- `soh.exe`: `0f25fa93765babc084c6864a209655628c064dd55cd370c6fea96d52e45bb7a3`.
- `prism.dll`: `cb9712e11af9ebe96457dbf8f5daad4a6c359ae1f59cdf2663282b3a9cc9759c`.
- `soh.o2r`: `9c873da64f0b030d2357d7699944f302cd06cd6ab8b421cb31b53ebc2eecf9b5`.
- `debug/soh.pdb`: `6ff2f6ac372e02466b058a7a1a45d529f522c2788c4fb08daeb1fdb79514d506`.

The Release build, asset generation and packaging succeeded. All 7,937 package entries passed CRC validation, as did all 1,279 entries in the port archive. Packaged speech resources match the source banks. The executable imports Prism and Windows system DLLs; Prism's static imports are Windows system DLLs. The deployed executable, DLL and archive hashes match the package.

The standalone speech checks passed. A separate silent probe loaded the real Prism DLL and selected NVDA. These checks cover backend lifetime, interruption and queue flags, text preparation, localized positions and choice parsing. They do not confirm in-game behavior. The game was not launched or controlled during this speech work.

The previous installed build is preserved in `H:\projects\ocarina\backups\9.2.3-before-prism-20260921`. The extracted game archive was verified unchanged; saves and configuration were preserved. No system restart is needed. A separate fresh setup copy at `H:\projects\ocarina\setup-test-prism\soh.exe` allows extraction testing without moving the working game's assets. Neither the distribution ZIP nor this fresh copy contains ROM-derived game archives.

Your current manual checks are in [todo.md](../../todo.md). Logs are under `C:\Users\Amethyst\source\ocarina-build`: `configure-prism.log`, `build-prism-verified.log`, `generate-prism.log`, `package-prism.log` and `build-speech-checks.log`. Full artifact hashes are also saved in `prism-build-manifest.json` there.

## Release submodules

- OTRExporter: `32e088e28c8cdd055d4bb8f3f219d33ad37963f3`.
- ZAPDTR: `ee3397a365c5f350a60538c88f0643f155944836`.
- libultraship: `fdcaf6336776d24a6408d016b0a52243f108f250`.

## Build setup

The source stays on H:. Build files and the project-specific vcpkg checkout are on C: to reduce network-share traffic.

- Build directory: `C:\Users\Amethyst\source\ocarina-build\9.2.3`.
- Dependencies: `C:\Users\Amethyst\source\ocarina-build\vcpkg`.
- Generator: Visual Studio 17 2022, x64, v143.
- Compiler: MSVC 19.44.35228.0.
- Windows SDK: 10.0.26100.0.
- Configuration: Release, with `BUILD_REMOTE_CONTROL=1` as in upstream Windows CI.
- CMake: Visual Studio 2022's bundled 3.31.6-msvc6.
- vcpkg revision: `5f96cd15fd745122cf27e0524606d6c1efc5fd07`, kept detached so the release's automatic `git pull` cannot advance it.

Set `VCPKG_VISUAL_STUDIO_PATH` to the 2022 BuildTools installation when configuring. Without it, vcpkg selected the newer 2026 installation independently of CMake. The initial 2026 dependency builds were removed using vcpkg and rebuilt with 2022.

The release's vcpkg helper still prints a failed `git pull` for the detached checkout. This is expected: the pinned dependency source remains unchanged.

## Rebuild commands

Run in PowerShell. These commands build tools and the game but do not launch the game.

```powershell
$env:VCPKG_VISUAL_STUDIO_PATH = 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools'
$env:VCPKG_MAX_CONCURRENCY = '8'
$shipCmake = 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
$shipBuild = 'C:/Users/Amethyst/source/ocarina-build/9.2.3'
& $shipCmake -S H:/projects/ocarina/Shipwright -B $shipBuild -G 'Visual Studio 17 2022' -T v143 -A x64 -DCMAKE_BUILD_TYPE:STRING=Release -DVCPKG_ROOT:PATH=C:/Users/Amethyst/source/ocarina-build/vcpkg -DBUILD_REMOTE_CONTROL=1
& $shipCmake --build $shipBuild --config Release --target GenerateSohOtr --parallel 4
& $shipCmake --build $shipBuild --config Release --parallel 4
```

Check each command's exit code before running the next. `GenerateSohOtr` runs the asset tool, not the game.

Configuration, `GenerateSohOtr`, and the full Release build all exited successfully on 21 September 2026.

For packaging, run the adjacent `cpack.exe` from the build directory with `-G ZIP -C Release -D CPACK_PACKAGE_FILE_NAME=Ship-9.2.3-prism-win64`. Upstream writes the ZIP into `H:\projects\ocarina\Shipwright\_packages`.

## Original baseline verification

- The original baseline matched the release tag. Submodule revisions remain unchanged in the current build.
- `soh.exe` is an x64 Windows GUI executable.
- The baseline executable's static import list contains only Windows system DLLs. The current build also imports Prism.
- The generated `soh.o2r` contains 1,279 entries; ZIP CRC validation passed.
- Its `portVersion` field decodes to 9.2.3, matching the exporter serialization in `OTRExporter/OTRExporter/Main.cpp`.
- Upstream compiler warnings remain, including numeric conversions and missing returns in existing stubs and resource helpers. No warning cleanup or gameplay changes were included in this baseline build.
- The native setup build was launched at Amethyst's request. Process metadata confirmed a responsive native window titled "No O2R Files". Amethyst subsequently confirmed that setup worked and the game ran. Current Prism speech still needs her in-game testing.

Original baseline SHA-256 values:

- `soh.exe`: `69AAFAF868CED1AB55BAEE9AF74D83D0B3B6B9B149C041D08FBCA49ED04AA310`.
- `soh.o2r`: `FED893FEA7A5C93C7C5EA2B1608A36986F34439189CB8EA29C3365931CB72860`.
- `gamecontrollerdb.txt`: `07EC5B753E685C4829987919B27905CC3C45C8E18C11B3046E275FCF38B0F3CB`.

The controller database is fetched from upstream's current master by the release build system; the hash above identifies the copy used here. These dependencies are the versions built on this machine, not a claim of byte-for-byte reproduction of upstream's published binaries.

Build logs are in `C:\Users\Amethyst\source\ocarina-build`: `configure-vs2022.log`, `generate-soh.log`, `build-release.log`, and `package-release.log`.

## Original prepared installation

The original baseline package is `_packages/Ship-9.2.3-win64-ship.zip`. All 7,912 ZIP entries passed CRC validation and the executable, port archive, controller database, README and debug symbols were present.

- ZIP SHA-256: `8AD13B76271EE6C4C2640140EC56C558066411468F3CB305EDC530651B7D4B59`.
- Extracted installation: `H:\projects\ocarina\game`.
- Launch path, for Amethyst: `H:\projects\ocarina\game\soh.exe`.

The destination was created new for the baseline. It now contains the Prism build described above. No system restart is required. The current testing checklist is `H:\projects\ocarina\todo.md`.

## Native first-run setup fix

The [9.2.3 build instructions](https://github.com/HarbourMasters/Shipwright/blob/9.2.3/docs/BUILDING.md) explicitly use the built-in extractor after compiling. No additional extractor-install step was missing. The [website's Windows guide](https://www.shipofharkinian.com/setup-guide/windows) still describes OS prompts, but upstream commit [`704ace8fd37e28b01b2ac4f5ec3d62a4362dcc95`](https://github.com/HarbourMasters/Shipwright/commit/704ace8fd37e28b01b2ac4f5ec3d62a4362dcc95), "ImGui-Driven Extraction Flow and Progress Reporting", changed these prompts on 15 January 2026.

The original `RunExtract` queued an ImGui "No O2R Files" question before reaching `Extractor::GetRomPathFromBox`, which already calls the Windows `GetOpenFileNameA` dialog. The official 9.2.3 executable contains both that same initial prompt text and the "Open Rom" dialog title. Its SHA-256 is `2092C24DA65B53A76C5D86F8D129B9CF5786FC9DEF29EC25B5B0B23D58B153A0`; the downloaded comparison ZIP is under `H:\projects\ocarina\research\upstream-9.2.3`.

`ShowExtractionPopup` now presents desktop setup prompts through `SDL_ShowMessageBox`, using the existing titles, messages, button captions and actions. The built SDL Windows backend uses native task dialogs, with a native dialog fallback. Console targets retain their existing ImGui prompts. Enter selects the first button; Escape selects the second when present, or acknowledges a one-button dialog. Native dialog creation failures log their reason and stop setup.

The normal ROM picker and ROM validation are unchanged. At this stage extraction progress remained visual and general in-game speech was unchanged; the current Prism build adds speech as described above. The completion question is native. Only Windows was built and runtime-checked; other platforms have not been tested.

- Native setup executable SHA-256: `6A89140FDA7612E6D2EB068C624552CD27D69C1F3B87944534A1B6A8A10C93DC`.
- Native setup package: `_packages/Ship-9.2.3-native-setup-win64-ship.zip`.
- Native setup package SHA-256: `88F2F59AB32E0E0EB9621F96315921DBC1BC697BA61C389775B4A949F8B7A83C`.
- Previous installed executable and symbols: `H:\projects\ocarina\backups\9.2.3-before-native-setup-20260921`.
- Build log: `C:\Users\Amethyst\source\ocarina-build\build-native-setup.log`.
- Packaging log: `C:\Users\Amethyst\source\ocarina-build\package-native-setup.log`.

The Release rebuild and CPack succeeded. The new ZIP passed CRC validation, and its executable matches the deployed executable. The game was launched from `H:\projects\ocarina\game` and left at its native setup prompt. The desktop automation helper failed to connect to its native pipe even after the prescribed retry/reset, so no accessibility-tree inspection or file-picker interaction was performed.

## Game assets and testing

The game assets are separate from the source. Supported ROM SHA-1 hashes are listed in `docs/supportedHashes.json`. The offline extractor is `OTRExporter/extract_assets.py`; it can prepare assets without starting the game.

Amethyst clarified that end users must select their own ROM through first-run setup. This is recorded in `H:\projects\ocarina\setup-questions.md`; a ROM path is no longer requested. Amethyst normally launches and tests the game, and explicitly authorized the earlier native setup launch. She has since confirmed that setup works and the game runs. Extraction speech and in-game Prism behavior still need her test using the current checklist.
