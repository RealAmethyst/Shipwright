# Cue repetition timing

Amethyst reported that build `05bfdd1a3a0e` sounds much better overall, but the person
recording repeats too slowly. She requested the looping behavior from Time Stranger.
The current installed build and listening checklist are in [todo.md](../../todo.md).

## Verified cause and reference

`CueActors.cpp` used PR 5435's game-frame intervals to call `CueMixer::Play`, which resets
the recording cursor. Most NPCs used 35 updates, with others using 20, 30 or 40. At the
normal 20 gameplay updates per second, those mean 1–2 seconds between starts, independent
of the replacement recording's length. The person WAV lasts only 0.388156 seconds. Other
policies restarted longer recordings before their end; Jabu's elevator restarted every
update. A fixed frame timer is not suitable for these replacement recordings.

The normal rate is verified by `OTRGlobals.cpp`'s `original_fps = 60 / R_UPDATE_RATE` and
the native play code restoring `R_UPDATE_RATE = 3`. Graphical interpolation does not change
the intended ordinary cue repetition rate.

Read-only reference source under `H:/projects/digimon story time stranger/mod`:

- `DSTS.Accessibility/Core/SpatialAudioEngine.cs`: `LoopGapSamples = 2646`,
  `FadeSamples = 88`, sample rate 44100. `FillMonoFrame` plays each ordinary clip in full,
  then waits 60 ms. Its boundary fades are approximately 2 ms. Wall loops instead overlap
  the final 2 ms with the head and resume after the overlap, without a silent dip.
- `DSTS.Accessibility/Core/SpatialAudioSources.cs`, `Place`: surviving entities retain
  their voice and playback cursor. Destructibles request a 0.4-second cycle. The engine
  truncates their roughly 1.02-second recording to that excerpt, fades the effective
  ending and inserts one silent sample before retriggering.
- `Handlers/FieldAudioHandler.cs` passes the field sources through that owner-preserving
  path. `Core/FieldNavigator.cs` supplies a separate distance-dependent interval for
  selected-route guidance. Ocarina's placed route markers are ordinary world beacons;
  this change does not introduce Time Stranger's selected-route navigation behavior.
- `SpatialAudioEngine.LoadWavMono` reads the full clip and resamples when required. It
  does not trim silent sections. The reference project and original recordings are unchanged.

## Implementation

Ordinary actor, exit, climbable-surface and fixed-location cues now use `KeepPlaying`.
Game updates maintain each source's availability, position and volume. Repetition runs
from the audio sample cursor, independent of game-frame/render-block boundaries. Obsolete
ordinary frame intervals were removed from the actor policies and location records.

At Ocarina's 32000 Hz, entity gaps are 1920 samples, fades/overlaps are 64 samples, and the
destructible excerpt is 12800 samples with a one-sample gap. Walls crossfade into the head
and resume at sample 64. Ordinary loops start on entering range; updating a moving source
retains its playback position. Disabling, loss of availability, pause or scene cleanup
releases the voice and its pending gap. Individual volumes, HRTF/surround and native reverb
use the same existing mixer path.

The PR's special-item approach pulses and silver-rupee target pulses remain distinct from
ordinary looping: their existing frame/distance signals still trigger one-shot playback.
No new timing control, navigation binding, automatic action or sound assignment is added.

Approximate start-to-start cycles for the bundled files:

- Person: 0.448 seconds.
- Item: 0.434 seconds for ordinary pickups/chests.
- Door: 0.644 seconds.
- Transition: 3.039 seconds, including its complete 2.979-second recording.
- Destructible: 0.400 seconds.
- Crawlspace: 0.488 seconds, using the ordinary entity-loop rule for this additional clip.
- Ladder: 1.357 seconds.
- Elevator: 1.084 seconds.
- Pathfinder marker: 0.185 seconds.
- Wall north: 3.118 seconds after the initial pass.
- Wall east: 3.078 seconds after the initial pass.
- Wall south: 4.643 seconds after the initial pass.
- Wall west: 2.278 seconds after the initial pass.

## Verification

The production cue mixer renders each of all 13 bundled recordings over multiple cycles.
Checks compare the output to the original PCM, verify the complete ordinary clip, exact
60 ms gap, destructible excerpt and gap, 2 ms fades and wall seam samples. Repeated source
updates preserve the cursor. Different irregular block sizes produce identical output.
Gain, stop/disable, changed recording, voice capacity, one-shot completion and scene cleanup
remain covered. The spatial, native-mixer and wall/compass suites also pass.

An initial new test compared past its output vector's end; that test bound was corrected
before the final passing run. No game was launched or controlled. The normal Release build
and package checks are recorded with the deployment in [accessibility-build.md](accessibility-build.md).
Actual listening, busy-area mixing and real-time performance remain Amethyst's checks.
