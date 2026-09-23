# Prism speech

This work extends Shipwright 9.2.3's existing speech. Amethyst confirmed that speech should default to enabled, the menu checkbox should remain, and extraction should announce its start, each new ten-percent milestone, and completion. F9 no longer toggles speech.

## Dependencies and startup

- The fresh-install crash after ROM selection on build `d4342490da07` came from GUI debugger callbacks querying a service that was initialized only after extraction. Build `3070a31e1701` initializes it before the window is created. Prism is not on the failing call path, and the speech DLL/code are unchanged. See [the dump and source trace](setup-crash-20260922.md); extraction speech still needs Amethyst's test with this build.
- The speech library is ethindp/Prism 0.18.2, source commit `f237af67d0460a6dad312cc446623856ad823c50`. It is distinct from libultraship's shader processor named prism.
- `CMake/PrismSpeech.cmake` downloads the official Windows x64 archive with a pinned SHA-256. Packaging includes its unmodified DLL and licenses. Other desktop builds require the matching official Prism SDK; consoles compile without a speech backend. Only Windows is validated here.
- The release's actual `PrismConfig` is version 3. `prism_config_init`, `prism_init`, and `prism_registry_create_best` establish the backend; `create_best` already initializes it. Do not call backend initialization a second time. Free the backend before shutting down the context.
- `InitOTR` initializes Prism and the shipped text banks after the port archive is mounted, before `RunExtract`. It reloads the banks after normal game initialization and language changes.
- All speech passes through `SpeechSynthesizer::PrepareText`. It preserves UTF-8 and punctuation and normalizes whitespace. The game-message decoder handles Ocarina's byte controls and glyphs before that boundary. Empty or missing text is silent; stopping speech uses an explicit `Stop` call.
- Prism receives the interrupt flag directly. There are no per-utterance detached threads and no game-owned SAPI, Darwin or eSpeak speech implementations. The selected reader owns its voice preferences.

## Verified source paths

- Extraction: `OTRGlobals::RunExtract` submits `Extractor::CallZapd`, displays the atomic extracted/total counters, and waits for its future before completion. Speech runs on the main thread. Progress counters are each loaded once per update, avoiding a denominator changing to zero during worker cleanup. Native setup/error dialogs remain native.
- Boot logos: `CustomLogoTitle_Main` chooses the libultraship or Nintendo logo. The new hook fires on the first visible frame and resets in `OnZTitleInitReplaceTitleMainWithCustom`. Skipped logos are not announced. The actual textures were decoded and inspected: “powered by libultraship” and “NINTENDO 64”.
- Game title: `EnMag_DrawInner` draws the Zelda logo, Ocarina of Time caption, optional Master Quest subtitle, and the start/controller prompt. `EnMag::speechFlags` is new port-owned state, reset by `EnMag_Init`, with the actor's native lifetime. The prompt uses the same `pressStartMsg`/`noControllerMsg` tables and translation condition as the renderer. No native offsets or binary hooks are used.
- File selection: `OnFileSelectUpdate` runs after the native menu update. `OnFileSelectClose` fires in `FileChoose_Destroy`. Native config/select modes identify menus; transition frames do not imply closure. Returning from a child retains the parent's opening announcement. The initial item comes from the live context.
- Native Options return: the Options overlay pauses FileChoose_Main without changing configMode or buttonIndex. The old reader's unchanged-item cache suppressed speech on return. Both actual close paths in NativeOptions_Update (B/Backspace through the model, and Escape through window visibility) now call TTSResumeFileSelect. MenuSpeech::Resume invalidates focused-item deduplication and old queue/hint state while preserving previousFileView and openedFileViews. The next verified OnFileSelectUpdate announces the live localized item and its position once, interrupting Options speech. It does not repeat the parent heading/hints, hardcode the Options row, or speak during the overlay. OnFileSelectClose still resets the real screen lifetime.
- File captions: `sTitleLabels`, `FileChoose_GetQuestChooseTitleTexName`, `FileChoose_GetSohOptionsTitleTexName`, and the name/options drawing functions select artwork. New wording stays in the existing localization banks. The custom quest/settings textures were inspected: “Please select a quest.” and “Please select your settings.” Boss Rush and Randomizer share the latter caption. The control texture reads “A-Decide, B-Cancel”. New file-menu artwork captions are supplied only in English: the installed game archive does not provide the original French/German artwork for verification. Those languages retain their existing item labels and gain localized positions; unverified new headings/hints stay silent rather than using guessed translations.
- Pause: `OnKaleidoscopeUpdate` follows the native update. State 0 closes the pause screen; state 6 with `unk_1E4 == 0` identifies a settled page. Its page caption precedes its current item. The first-item queue persists until the item is readable. Returning from save resumes the current page. L and R icons are drawn by `KaleidoScope_DrawInfoPanel` and queue after the first item on opening.
- Dialogue: `Message_Decode` in `z_message_PAL.c` establishes `choiceNum`. Its renderer establishes `choiceIndex`, including Better Owl's alternate default, before `MSGMODE_TEXT_DONE`. Speech separates the question from choices, queues the first selected answer after the question, and interrupts on movement. Incomplete choice lists remain silent. Byte `0x80` is the accented character À, not ASCII.

## Camera compass and speech settings

The existing speech switch now lives under Accessibility, Text to Speech, preserving
`gSettings.A11yTTS`. The submenu also has Spoken compass (default On), Compass directions
(four by default, optional eight), and Reset speech settings. Reset clears the three keys
through normal CVar change/save handling. Turning speech off also silences the compass.

`CompassSpeech.cpp` reads the actual view's look-at minus eye vector once per gameplay frame.
It follows the verified map axes, including mirrored-map reflection, for the 20 outdoor maps
and ten mapped dungeons in `WorldCompass.cpp`. Dungeon directions describe the local map;
unmapped interiors and separate boss arenas stay silent. Direction words are centralized in
the English, French and German misc banks and use `TTSSpeakLocalized` with queued Prism speech.

The first valid heading establishes a silent baseline. A changed heading is announced after
three steady samples. Menus, dialogue, cutscenes, invalid camera data and unsupported scenes
clear the baseline. First-person aiming remains usable for compass speech. Production heading
tests cover four/eight directions, reflection, wrapping, settling, jitter and invalid input;
real camera timing and listening remain in the root checklist. No compass binding or PR
orientation bell is introduced. See [wall-compass-research.md](wall-compass-research.md).

## Positions

Positions are last in the item utterance and use the central `position` resource. Quantities precede item names.

- Main file menu: native indices 0–5, six reachable entries, including unavailable actions which the native cursor still reaches.
- Copy source and erase: indices 0–3, including Quit. Copy destination excludes `selectedFileIndex`, leaving three reachable entries.
- File confirmations and save/continue prompts: two choices.
- Options: two setting rows, or three for PAL N64; the setting row supplies the position, and its selected value remains in the utterance.
- Quest choice: from `MIN_QUEST` to `QUEST_BOSSRUSH`; skip Master Quest when its archive is absent, and omit Original when absent.
- Boss Rush: `BR_OPTIONS_MAX`; Randomizer: `RSM_START_RANDOMIZER` through `RSM_OPEN_RANDOMIZER_SETTINGS`.
- English name entry: 65 character cells plus Backspace and End. The displayed PAL/NTSC keyboard tables provide character codes. Japanese name entry and dialogue remain unsupported and are kept silent instead of being misdecoded.
- Pause items: the 24 inventory slots, excluding empty slots unless Pause Any Cursor permits them. Equipment uses the native equipment bits, upgrades and Pause Any Cursor setting. Quest status has 25 cursor points: `KaleidoScope_UpdateQuestStatusPoint` accepts empty points too. Unknown item captions remain silent.
- World map: the 12 native points whose `worldMapPoints` entry is nonzero. Dungeon map: owned dungeon items and the eight floors that are visited or revealed by the map. Page controls are separate from the item list.

## Offline verification

`tests/speech` builds independently with CMake. Set `PRISM_SDK` to the extracted official release and `CMAKE_PREFIX_PATH` to the existing vcpkg triplet. `speech_checks` exercises the production speech bridge with a recording backend: initialization, cleanup, interrupt/queue order, missing text, UTF-8, localized positions and two/three-choice parsing. `prism_probe` loads the real DLL and reports its selected backend without speaking, stopping speech or launching the game.

The MenuSpeech regression tests additionally cover returning to an unchanged file-select item, repeated opens/returns,
missing text on return, Escape over another row or confirmation, child-screen returns, real screen reopening and
delayed initial-item queues. The runtime close paths were traced to the native post-update hook; tests do not launch
the game. Amethyst's focused file-select return check remains in the root todo.md.

The first real probe selected NVDA. Game behavior, extraction announcements, timing and controller navigation still require Amethyst's tests listed in the project `todo.md`.
