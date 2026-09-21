# Accessibility fork: build and installation

## Source

- Fork: https://github.com/RealAmethyst/Shipwright
- Upstream: https://github.com/HarbourMasters/Shipwright
- Starting release: Ackbar Delta 9.2.3, published 14 April 2026.
- Release commit: `cb71e22a79bc5d1f688fa881795bbd93094895fc`.
- Local branch: `accessibility`, created directly at that tag.
- Checkout: `H:\projects\ocarina\Shipwright`.

GitHub verified RealAmethyst/Shipwright as a fork of HarbourMasters/Shipwright on 21 September 2026. Origin points to the fork and upstream to HarbourMasters. The accessibility work is recorded in local commits only; nothing has been pushed. Local changes restore native desktop setup dialogs and integrate Prism speech for extraction, boot logos and the existing game TTS, including menu positions and announcement ordering. Wider gameplay accessibility remains future work.

## Current native Options build

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

## Local accessibility checkpoint

The Shipwright accessibility branch includes the native setup, Prism and native Options work. Its libultraship
submodule points to local commit `0eeb24cbfb03f324e342ee95ff84087b765e2383` on that submodule's `accessibility` branch.
The source contents were built before recording the commits; the installed executable is identified by the hash
above, and its embedded release base still says cb71e22. Both commits include Codex attribution.

No commits have been pushed. The library commit exists locally under Shipwright's submodule repository; publishing
the superproject later also requires making that submodule commit available from an appropriate remote. The current
GitHub fork contains the upstream release until the user authorizes publication. Game data, downloaded dependencies,
logs, build outputs and distribution archives are excluded from the commits.

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
