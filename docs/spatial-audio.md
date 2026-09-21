# Spatial audio integration

Built, packaged and deployed on 22 September 2026. Installed executable ID: `d4342490da07`.
Headphone HRTF, Windows spatial output, thirteen assigned recordings, three wall probes and
the camera compass are implemented. See [wall-compass-research.md](wall-compass-research.md).
Other cue meanings await assignments in `../../spatial-sound-assignments.md`. Listening tests
remain in `../../todo.md`; no game has been launched or controlled by Codex for this work.
Amethyst reports an overall improvement; her slow person-cue feedback is addressed in
[cue-loop-timing.md](cue-loop-timing.md), with Time Stranger repetition for ordinary recordings.

## Scope

Preserve original Stereo/Mono, provide Steam Audio HRTF for Headphone, and provide real spatial
output for Surround. Music and nonpositional interface sounds retain their original mix.
Bundled accessibility cues join the same mixer, including its native area reverb. Replicate
the detection and timing logic in PR 5435 with Amethyst's own sound assignments, never its
extracted game recordings or setup flow. Each assigned cue needs its own toggle and volume.

## Verified sources

- PR 5435 was open on 21 September 2026, head `257c6c0dfdf9cf9330735b43bc7512e1026d6fae`,
  branch `serprex:actor-accessibility-experiments`. Its 25-file diff and reconstructed new
  files are preserved in `C:/Users/Amethyst/source/ocarina-build/spatial-audio-20260921`.
- `func_800F6700` maps settings 0/1/2/3 to synthesis modes 0/3/1/0 and spatial-property modes
  (`D_80130604`) 0/3/1/2 in the original code. Original Surround selected libultraship `audioMatrix51`.
- `Audio_InitNoteSub` implements Headphone with `gHeadsetPanQuantization`,
  `gHeadsetPanVolume`, and per-ear delays applied by `AudioSynth_NoteApplyHeadsetPanEffects`.
  It is not an HRTF. Mono has equal 0.707 left/right gains.
- `Audio_SetSoundProperties` computes channel volume, reverb, pan, pitch and filters from
  a `SoundBankEntry`. Surround additionally encodes phase/rear effects and vertical filtering.
  libultraship `SoundMatrixDecoder::Process` subsequently upmixes the whole stereo mix to 5.1,
  including music. This cannot recover individual source positions.
- `Audio_PlayActiveSounds` connects `gActiveSounds[bank][slot]` to
  `gSoundBanks[bank][entry]` and `seqPlayers[SEQ_PLAYER_SFX].channels[channel]`.
  `Audio_NoteInitForLayer` connects a synthesized note to its sequence layer and channel.
  `Audio_ProcessNotes` snapshots those notes for each update, before any synthesis slices run.
  Released notes retain copied attributes after `parentLayer` becomes `NO_LAYER`; a new
  channel occupant must not move a previous note's release tail.
- `AudioSynth_ProcessNote` decodes/resamples samples, applies gain and native filters, then
  calls `AudioSynth_ProcessEnvelope`. `aEnvMixerImpl` applies gain ramps and writes distinct
  direct and wet buffers. Spatializing here can preserve the existing reverb send without
  applying HRTF to music. Cue wet sends must enter before `AudioSynth_SaveReverbSamples`.
- Normal specs use 16–28 notes and one or two reverbs (`audio_init_params.c`). The output
  device is 32 kHz (`OTRGlobals::InitAudio`). `OTRAudio_Thread` synthesizes blocks of 528 or
  560 frames, up to three blocks per update. HRTF block adaptation must preserve samples
  across these changing sizes, and delay the unprocessed bed consistently.
- `Graph_Update` finishes `GameState_Update`, then runs `func_800F3054`. Later,
  `Graph_ProcessGfxCommands` wakes the audio worker and waits for completion before returning.
  Graphics command execution overlaps audio; game state simulation does not. New audio
  metadata must be published before waking the worker, and must not depend on GUI-thread
  settings reads or rendering mutations during playback.
- Actor and fixed-world-source sound positions are projected coordinates, not raw world
  coordinates. `Actor_Draw` and `SoundSource_UpdateAll/PlaySfxAtFixedWorldPos` call
  `SkinMatrix_Vec3fMtxFMultXYZ(W)` with `play->viewProjectionMtxF`. That function writes
  `M * [world, 1]` without dividing by W. Feeding those XYZ values directly to HRTF is wrong.
  Capture the world source at the verified projection producer, with explicit lifetime and
  value validation. Nonworld sources such as `gSfxDefaultPos` must stay in the ordinary bed.

## External audio implementation

Steam Audio 4.8.1 official SDK ZIP SHA-256:
`4a0aa5ec1176f38f0b0993a37c2259d9e86f27e22d5e24f83ec4c3cb9a1d5449`.
The downloaded SDK and its matching header have been verified. Its coordinates are right,
up, negative Z forward. Use the public C API and ship the pinned runtime and notices.

`iplBinauralEffectApply` consumes individual mono/stereo sources and outputs stereo; it
does not open an output device. Stream tails and source identities must survive arbitrary
game block sizes. HRTF is initialized off the real-time output callback.

Steam Audio is not an Atmos encoder. Windows `ISpatialAudioClient` delegates spatial
rendering to the selected platform provider (Sonic, Atmos or DTS). It supports dynamic
objects and static height channels. Availability, object counts, format changes and device
loss must be checked and reported. A matrix upmix is not an acceptable substitute for
claiming height/object output.

The SDK's 4.8.1 `PanningEffect::apply` was also inspected: its interpolation loop uses the
channel index rather than the sample index for alpha. Its custom-speaker path is first-order
diffuse panning. Do not assume that selecting a custom 7.1.4 layout provides precise object
rendering; this needs explicit handling and offline directional verification.

Official references:

- [PR 5435](https://github.com/HarbourMasters/Shipwright/pull/5435).
- [Steam Audio integration](https://valvesoftware.github.io/steam-audio/doc/capi/integration.html).
- [Binaural effects](https://valvesoftware.github.io/steam-audio/doc/capi/binaural-effect.html).
- [Windows spatial sound](https://learn.microsoft.com/en-us/windows/win32/coreaudio/spatial-sound).
- [Windows spatial-object rendering](https://learn.microsoft.com/en-us/windows/win32/coreaudio/render-spatial-sound-using-spatial-audio-objects).

## Implementation and verification sequence

1. Trace source identity, world transforms, note snapshots, room reverb and thread boundaries.
2. Build a bounded streaming mixer with real Steam Audio, preserving the ordinary bed.
3. Connect source snapshots and native wet sends; verify unrelated output is unchanged.
4. Implement and probe platform spatial output without starting the game.
5. Port the PR's supported cue logic with explicit asset mappings and central localization.
6. Run offline waveform, lifecycle, block-size, reverb, settings and regression checks.
7. Build/package, back up the installed files, deploy verified changes and refresh todo.md.

## Completed offline work

The first streaming HRTF mixer is implemented in `BinauralMixer` and tested with the actual
Steam Audio DLL. Directional impulses distinguish left/right, front/back and above/below.
Splitting identical input into 16-frame slices or the game's irregular 176/192/208-frame
slices produces the same waveform. The stereo bed is bit-identical after its fixed 128-frame
buffering delay. Invalid directions, nonfinite PCM and invalid slice state are rejected.

The first real SDK call exposed a compatibility issue: the compiled default HRTF does not
support 32 kHz (`HRTFMap::loadNumSamplesForSamplingRate`, which throws Initialization when
the rate is absent). Valve's `hrtf.cpp` names CIPIC subject 124 as the default. Its matching
SOFA source is distributed under the notice in SDK `THIRDPARTY.md`. `SOFAHRTFMap` calls
`mysofa_open_data_no_norm` with the requested rate, providing supported resampling at load
time. This path passed the 32 kHz tests without resampling the original game mix.

SOFA SHA-256: `c28ff4a874ac889ec0c5885ca524762a70d56984232ff7aadcd9c15d32e1cfb6`.
Runtime integration and Windows spatial output now build successfully. The thirteen assigned
recordings, including four wall directions, cover 73 actor kinds, collision-derived exits/climbable surfaces and 65 unique
crawlspace/route locations. The complete
PR sound inventory and unassigned meanings are in [spatial-cue-inventory.md](spatial-cue-inventory.md).
The other cue meanings still require Amethyst's assignments; they are not silently substituted.

## Native integration and cue verification

`SpatialAudio` captures projection producers in a bounded 4096-slot table without allocating
per projection. A sound-bank entry gets a stable identity when bound to an SFX channel.
The identity and camera-space direction are copied into each NoteSubEu snapshot. Removal
clears the channel binding; already-released notes retain their own snapshot. Failed current
projection validation clears the current direction and retains the native direct mix, with
a named diagnostic for the SFX ID. This makes remaining source-coverage gaps testable.

Additional producer tracing found copied positions which do not pass through the projection
function themselves. `EnHorse_PostDraw` supplies its actual limb-13 world position for the
separate `unk_21C` voice-position copy. `EnHorseNormal_Update` associates its modified `unk_204`
projection with the native focus position; `func_80A6CC88` associates the cloned horse's
modified `unk_1F4` with its supplied clone world origin. Its old projected Y increment is
not treated as a physical world-height measurement. `BossVa_SpawnTumor` instead passes
`effect->pos` directly: the source is parent world position plus effect offset, refreshed
at that same calculation in `BossVa_UpdateEffects`. These registrations leave the native
vectors untouched, preserving Stereo/Mono calculations. Twinrova beam origins and Iron
Knuckle's death-position vector already use the captured projection producer. Volvagia's
constant `{0, 0, 50}` fire audio vector is synthetic listener-space data, with no world
coordinate supplied to that sound; it remains in the native bed.

`Audio_ChooseActiveSfx` computes priority distance before `Audio_SetSoundProperties` takes
its square root. In Headphone/Surround only, verified sources use squared distance from Link
at that exact producer. Stereo and Mono keep the original calculations. `Actor_DrawAll`
projects actors before its drawing/frustum test (`z_actor.c` around 3064); the projection
capture therefore works for actors behind the camera as well.

The native mixer captures each world's mono voice after decoding, resampling and filters,
using the actual envelope gains, before dry/wet separation. Its wet sends continue through
the existing reverb. Nonpositional/music voices remain in the stereo bed. The production
mixer test checks untouched bypass/music, accepted capture, unchanged wet sends, rejected
capture fallback, and envelope ramps.

For bundled cues, `AudioSynth_Update` snapshots the SFX sequence's channel-zero `reverbIndex`
after each `AudioSeq_ProcessSequences` update. The native note consumer is
`Audio_NoteInitForLayer`, which uses `layer->channel->reverbIndex & 3`. Cue wet sends join that
same current bus after its notes and before its filter/save operations in
`AudioSynth_DoOneAudioUpdate`. Invalid/missing bus data yields dry-only cues.
`Audio_AccessibilityReverb` uses the same `sAudioEnvReverb + sAudioCodeReverb + sSpecReverb`
as `Audio_ComputeSoundReverb`, clamped to 127 and normalized by the native envelope's 128.
Room echo reaches `sAudioEnvReverb` through `z_room.c`. No replacement room/reverb engine
or invented geometry simulation is added.

Cue gain respects the live SFX sequence's game/fade gain and native master gain. Each
recording has an enabled CVar and a 0–100 volume CVar under `gSettings.A11yAudio`, exposed
through normal Native Options widgets under Accessibility, Audio. Defaults are On/10.
Native CVar persistence applies, with a reset action for each sound. The thirteen PCM recordings are loaded at startup,
validated as mono PCM16/32 kHz, and rendered in a fixed 128-voice pool.

Actor creation/destruction and Play destruction use native hooks. Positional updates run
with the existing game-frame/audio handoff. Options, pause, cutscenes, disabling, actor
removal and scene teardown stop cue voices. Item availability checks use the actual typed
actor action functions, including the two different Deku Baba drop functions and the
collectible's already-picked-up animation. Scene exit bounds come from the actual
`Scene_CommandExitList` resource's `GetPointerSize`, matched to `play->setupExitList`.

## Additional assigned cue sources

Amethyst subsequently assigned the destructible, crawlspace, ladder, elevator and pathfinder
recordings, plus person for gossip stones. `CueNames` drives matching toggles and volume
rows; all thirteen recordings, including the later wall assignments, are decoded and checked for non-silent playback and completion
offline. Their source hashes and conversion details are in the dependency manifest.

`EnKusa_Main` identifies intact, interactable grass; cut/regrowing stumps stay silent.
`EnWood02` values below `WOOD_LEAF_GREEN` cover the PR's trees/bushes, excluding leaf particles.
Held breakables are excluded. Jabu's `BgBdanObjects_Init` masks the type to its low byte;
type 2 is its lift. `BgYdanMaruta_Init` separately normalizes type 1 for the Slingshot Room
ladder. Forest elevator detection retains the PR's 300-unit range and one-unit height band.

Native `func_80041DB8` reads the wall-property table and is consumed by the player's climbing
state (`func_8083EC18` and `sTouchedWallFlags`). It is the verified 9.2.3 equivalent of the
PR's `SurfaceType_GetWallFlags`. Flags 8/3 create ladder points from the actual collision
triangles, using the PR's bounds-center/minimum-Y calculation. Native `yDistToWater` is
water-surface Y minus actor Y (`z_actor.c`), or `BGCHECK_Y_MIN` without water; ladder points
can therefore follow the current water surface without retaining a stale raised height.

`CueLocations.h` records 65 unique PR positions: four crawlspaces and 61 route/conditional
markers. Seven exact duplicates are merged; the separately unassigned sword pedestal is
omitted. Ranges come from the pinned PR. Replacement recordings now use the sample-based
loops documented in [cue-loop-timing.md](cue-loop-timing.md). Forest basement points follow the
actual rotating-wall actor's world angle. Twisted-hallway points follow its normalized native
type. Original-layout dungeon points are suppressed when the actual scene selects Master
Quest resources; outdoor points remain available. Hookshot surfaces and other unassigned
signals are not inferred from the pathfinder assignment.

Source conversion/import records are `convert_more_cues.py`, `import_cue_locations.py` and
`imported-cue-locations.json` in the local build research directory.

## Windows speaker output and limits

The new libultraship `SpatialAudioPlayer` sends a real 7.1.4 static-object bed through
`ISpatialAudioClient`. Constant-power vector panning routes world sources to the horizontal
and height speakers. The native music/nonpositional/reverb bed goes to front left/right.
There is no final-mix matrix upmixer. A 7.1.4 bed cannot reproduce a physically separate
speaker below the listener; below sources retain azimuth on its horizontal ring. Headphone
HRTF retains above/below differences.

Windows accepts 48 kHz float mono objects here. Twelve synchronized SDL audio streams
resample the 32 kHz channels to that device rate. A bounded queue feeds an event-driven
output thread. The thread owns COM, handles stop/join, clears stale audio on device loss,
and reopens the default device. Failed initial output activation retains the prior player
and sound mode and presents a localized Native Options error. The ordinary backend
preference is preserved when switching to/from spatial output.

This machine reported zero dynamic objects and a static mask of `0xffffe`. The static
7.1.4 stream was accepted. The production backend subsequently rendered 72,000 silent
frames successfully in an offline stream test. This proves API/render-loop operation,
not that Atmos or physical height speakers are enabled. Enable the intended Windows
provider/device for listening tests. No provider or Windows setting was changed.

Music is never assigned a world location or processed by Steam Audio. With Surround,
Microsoft explicitly documents that a headphone spatial provider can externalize even
the stereo bed. Its final processing cannot be described as unchanged music at the ears.
Headphone mode retains the native stereo music bed outside Steam Audio. See
[Microsoft's spatial-sound platform behavior](https://learn.microsoft.com/en-us/windows/win32/coreaudio/spatial-sound).
Windows speaker output is implemented; macOS/Linux Steam Audio paths are configured but
not built on this machine. Other targets keep native audio support and report unavailable
HRTF/spatial output instead of claiming support.

## Current offline results

- Four spatial/cue/native-mixer/wall-compass suites pass. Audio checks use the actual Steam Audio 4.8.1 DLL.
- Production wall-probe fixtures verify geometry classification, map bearings, supported scenes,
  bounded scans, dynamic collision owner IDs and no mutation of Link. Compass settling and
  mirrored/eight-direction modes pass. Wall recording loops retain their cursor and stop correctly.
- A sweep covers every whole-degree azimuth/elevation, with finite unit-energy gains,
  no directional gaps, no accidental LFE feed, and isolated front/side/overhead routing.
- HRTF capacity, release tails and identity reuse pass. Sixty-four continuous sources
  rendered one second of audio in 61 milliseconds in the local Release check.
- Bundled cue decoding, irregular block continuity, 10 percent gain, disabling, bounded
  capacity, completion and scene cleanup pass.
- Existing Native Options navigation/overlay suites and the speech suite passed for the previous
  build; their code and resources are unchanged in the cue-timing update.
- The final Windows Release build, port-asset generation and CPack succeeded. All 7,961
  package entries and 1,280 port-archive entries passed CRC checks. Packaged speech resources
  match source; the thirteen recordings, SOFA data, pinned DLLs and notices were verified.
- Deployment hashes match the package. The previous installation is preserved in
  `../../backups/9.2.3-before-cue-loops-20260922`. Archives, saves, configuration, Prism
  and overlay layout were verified unchanged. In-game listening verification remains pending.
- `phonon.dll` imports Windows system DLLs. OpenCL/GPUUtilities/TrueAudioNext are delay-load
  GPU paths; the implemented CPU binaural path ran with only phonon.dll present.

No game has been launched, controlled or closed by Codex for this work. Amethyst authorized
committing and publishing the source changes on 22 September 2026. The release guide remains
outside the repository; see the publication notes in [accessibility-build.md](accessibility-build.md).
