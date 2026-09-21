# Accessibility sound inventory

Thirteen recordings are implemented: item, person, door, transition, destructible, crawlspace, ladder, elevator, pathfinder, wall north, wall east, wall south and wall west. The wall cues use the PR's three probes; the camera compass defaults to four spoken directions, with an eight-direction option, following [Amethyst's answers](../../wall-compass-questions.md). There are 15 distinct available recordings: 14 from Time Stranger and the additional crawlspace recording supplied in Ocarina's research folder. The installed build and listening checks are in [todo.md](../../todo.md).

PR 5435 references 110 original game sound IDs in its cue logic. It declares 103 policies and registers 161 real actor kinds plus 7 virtual kinds. These are different counts; several meanings can share one replacement recording.

The current port covers 73 real actor kinds, collision-polygon exits and climbable surfaces, 65 distinct fixed or conditional crawlspace/route locations, and the three wall probes. Unassigned terrain, aiming, puzzle, combat and other policies remain pending.

## Assigned recordings

Ordinary recordings use Time Stranger's loop timing: full clip plus 60 ms, with its
0.4-second destructible beat and smooth wall seams. See [cue-loop-timing.md](cue-loop-timing.md)
for each recording's duration and the preserved special-item pulse behavior.

- Item: ordinary pickups, available unopened chests, collectible healing fairies, blue fire, dropped sticks, heart containers, Skulltula tokens, special-item approach pulses and silver-rupee targets.
- Person: supported NPCs, friendly guards and gossip stones. Catching guards remain unassigned.
- Door: ordinary doors, shutters and locked/barred/ice doors.
- Transition: supported loading zones, open grotto entrances and collision-polygon exits. Destination-specific PR recordings use this one replacement.
- Destructible: bushes, trees, small and large crates, and pots. Cut grass stumps, leaf particles and held objects are excluded. Bombable rocks/walls remain pending because the answer says “maybe”.
- Crawlspace: the PR's four placed entrances in Kokiri Forest and the Deku Tree, using the new research/crawlspace.wav recording.
- Ladder: native climbable collision surfaces and the Slingshot Room ladder. Water-level positioning follows the PR.
- Elevator: Jabu-Jabu's lift and the Forest Temple elevator, with the PR's proximity/state rules. Other moving platforms have a separate unanswered assignment.
- Pathfinder: the PR's placed route markers, including moving Forest Temple basement points and conditional twisted-hallway markers. The separately unassigned sword pedestal and hookshot-surface signals stay silent.
- Wall north/east/south/west: blocking walls found by the three Link-relative floor-following probes. The recording identifies the wall's map direction relative to Link, while audible position follows the camera. Full recordings repeat while detected. Each has its own switch, volume and reset. The 20 outdoor maps and ten mapped dungeons are supported; first-person aiming suppresses these wall tones.

## Available recordings still needing an assignment

Enemy and main quest remain unassigned. See [wall and compass findings](wall-compass-research.md) for the implemented wall behavior, corrected PR defects, Time Stranger comparison, original map/directional-text evidence and checked exit coordinates.

Mono/stereo copies are alternate encodings, not extra sound choices. The destructible source is named `distructive.wav`; the navigation source is `pathfinding_tracker.wav`. Original files are preserved. Conversion provenance and hashes are in `dependencies/accessibility-audio/manifest.json`.

## Meanings awaiting your answer

There are 88 blank lines and 1 tentative answer in [your assignment file](../../spatial-sound-assignments.md). The wall/compass behavior questions are answered. Reuse an available recording, name another recording or mark a meaning silent. These are design choices; no recording is substituted on your behalf. The unassigned compass/orientation chime is the PR's bell, separate from the implemented spoken compass.

- Small rocks you can interact with.
- Bronze boulders.
- Dogs, including the following-dog state.
- Horses.
- Cows.
- Cuccos, including the held/chasing distinction.
- Signs and readable stone signs.
- Bean planting patches.
- Graveyard digging spots.
- Frogs' music-game location.
- Big Poe spawn locations.
- Haunted Wasteland guide Poe.
- Wasteland flag poles.
- Desert oasis.
- Market drawbridge.
- Song of Time blocks.
- Moving platforms, including Water Temple hookshot platform and Deku Tree platform.
- Collapsing platforms.
- Pushable blocks.
- Sliding ice blocks.
- Icebergs and red-ice obstacles.
- Scarecrows and available scarecrow summon points.
- Hookshot pillars and hookshot surfaces.
- Master Sword pedestal.
- Bomb flowers.
- Non-interactable Kokiri Forest rocks.
- Lake Hylia scene objects that provide aiming cues.
- Chest alignment: standing at the usable front of the chest.
- Chest alignment: standing behind the chest; this is separate from the removed camera-behind pitch effect.
- Ordinary floor switches.
- Rusty hammer switches.
- Eye switches.
- Crystal/diamond switches.
- Jabu-Jabu switches.
- Sunlight switches.
- Light beams.
- Lake Hylia sun target.
- Invisible or conditional wonder-item trigger points.
- Ocarina/song activation spots.
- Torches: unlit.
- Torches: lit.
- Poe-associated torches.
- Spider webs.
- Lens of Truth spinning-room signals.
- Forest Temple basement gate.
- Poe painting/puzzle objects, including block/timer states.
- The three Deku Scrubs' 2-3-1 order signals.
- Hammer totem poles.
- Statue eye targets.
- Jabu-Jabu dungeon entrance/body.
- Jabu-Jabu tentacle obstructions.
- Catching guards' location.
- Catching guards' changing detection/facing state.
- False/killer doors.
- Keese.
- Gold Skulltulas and wall Skulltulas.
- Big Skulltulas, including vulnerable/armored state.
- Gohma larva eggs.
- Small jellyfish.
- Stingers, including submerged/emerging state.
- Bubbles and Anubis enemies.
- Tentacle enemies.
- ReDeads.
- Freezards, including active/inactive state.
- Iron Knuckles, including awake/asleep state.
- Beamos.
- Armos statues.
- Poe sisters.
- Phantom Ganon's appearance/attack state.
- Phantom Ganon's energy ball.
- Gohma's targeting/vulnerability state.
- Aiming alignment and changing vertical accuracy for ranged tools.
- Hookshot contact/range indication.
- Uphill slope.
- Downhill slope.
- Ledge/drop ahead.
- Raised platform/step.
- Spikes.
- Water.
- Ground/floor under the probe.
- Lava.
- Void/fatal drop or current.
- Fire circles/pillars.
- Wall contact with almost no movement.
- Wall contact with slow movement.
- Wall contact while moving along the wall.
- Compass/orientation chime.
- Burning Deku Stick running out of time.
- Bombable rocks and walls. Your tentative answer: distructible maybe?

## Placed-marker limits

The 65 positions/ranges are traced to the pinned PR and retained in `CueLocations.h`. Seven exact duplicate markers are merged, and the separately unassigned Master Sword pedestal is omitted. Original-layout dungeon points are disabled for Master Quest; active collision-derived ladder/exit cues and actor cues still work from the loaded data. Coordinates and useful audibility need in-game verification.

`func_80041DB8` is 9.2.3's wall-flags accessor; its bitfield table and the player's climbing consumer were checked before substituting it for the newer PR's `SurfaceType_GetWallFlags`. No memory offsets or scans are used. The rotating-wall and twisted-hallway consumers were checked against their native actor state.

## Source and implementation differences

The source is [PR 5435](https://github.com/HarbourMasters/Shipwright/pull/5435), head `257c6c0dfdf9cf9330735b43bc7512e1026d6fae`.

- The PR's separate miniaudio engine, extraction flow, glossary speech and automatic dog-following state change are not imported.
- Camera direction and Link distance follow Amethyst's answers. No artificial behind-camera pitch change is used.
- Original recording-specific base pitch/volume tuning is not imposed on the replacement recordings. Their default is the approved 10 percent.
- The PR gates most cues on isDrawn. Native Actor_DrawAll sets that flag after frustum culling, so it cannot establish that a source behind the camera does not exist. The port checks native initialization, draw/update availability, room and relevant item states instead.
- The two Deku Baba actor types have different structures and drop functions. The port checks their own typed action functions.
- The PR's VA_DOOR callback dereferences its null native actor. It does not create any VA_DOOR instances in its current list; that unsafe callback is not carried into this port.
- Completed/disabled/out-of-range/removed cues stop. Scene teardown clears identities. Starting phases do not consume game RNG.

## Original PR sound IDs

These identify the PR recordings only. None is used as a bundled audio source. A single ID can serve several meanings; assignments belong to the meaning above.

- `NA_SE_EN_AMOS_WAVE`. AccessibleActorList.cpp: 1089.
- `NA_SE_EN_ANUBIS_FIRE`. AccessibleActorList.cpp: 815, 823, 831.
- `NA_SE_EN_BALINADE_THUNDER`. AccessibleActorList.cpp: 1019.
- `NA_SE_EN_BIMOS_AIM`. AccessibleActorList.cpp: 1052.
- `NA_SE_EN_BIRI_FLY`. AccessibleActorList.cpp: 999.
- `NA_SE_EN_DAIOCTA_DEAD`. AccessibleActorList.cpp: 1195.
- `NA_SE_EN_DAIOCTA_SPLASH`. AccessibleActorList.cpp: 1015.
- `NA_SE_EN_DARUNIA_HIT_BREAST`. AccessibleActorList.cpp: 210.
- `NA_SE_EN_DODO_K_LAVA`. AccessibleActorList.cpp: 208.
- `NA_SE_EN_DODO_K_ROLL`. AccessibleActorList.cpp: 206.
- `NA_SE_EN_DODO_M_EAT`. AccessibleActorList.cpp: 594.
- `NA_SE_EN_FREEZAD_DEAD`. AccessibleActorList.cpp: 1037.
- `NA_SE_EN_GERUDOFT_BREATH`. AccessibleActorList.cpp: 1046.
- `NA_SE_EN_GOMA_BJR_EGG1`. AccessibleActorList.cpp: 990.
- `NA_SE_EN_KDOOR_WAVE`. AccessibleActorList.cpp: 688.
- `NA_SE_EN_MUSI_SINK`. AccessibleActorList.cpp: 188, 572, 1104.
- `NA_SE_EN_NUTS_DAMAGE`. AccessibleActorList.cpp: 558, 579, 970, 1054.
- `NA_SE_EN_NUTS_FAINT`. AccessibleActorList.cpp: 904, 907, 910.
- `NA_SE_EN_OCTAROCK_ROCK`. AccessibleActorList.cpp: 276, 841, 959.
- `NA_SE_EN_OWL_FLUTTER`. AccessibleActorList.cpp: 564, 1025.
- `NA_SE_EN_PO_APPEAR`. AccessibleActorList.cpp: 192, 855.
- `NA_SE_EN_PO_BIG_GET`. AccessibleActorList.cpp: 599.
- `NA_SE_EN_PO_CRY`. AccessibleActorList.cpp: 609, 865.
- `NA_SE_EN_REDEAD_CRY`. AccessibleActorList.cpp: 1032.
- `NA_SE_EN_STALTU_LAUGH`. AccessibleActorList.cpp: 173, 983.
- `NA_SE_EV_BLOCK_SHAKE`. AccessibleActorList.cpp: 894, 1388.
- `NA_SE_EV_BOMB_BOUND`. AccessibleActorList.cpp: 1087.
- `NA_SE_EV_BOMB_DROP_WATER`. AccessibleActorList.cpp: 1067.
- `NA_SE_EV_BRIDGE_OPEN`. AccessibleActorList.cpp: 204, 604.
- `NA_SE_EV_BUTTERFRY_TO_FAIRY`. AccessibleActorList.cpp: 587.
- `NA_SE_EV_CHAIN_KEY_UNLOCK`. AccessibleActorList.cpp: 933.
- `NA_SE_EV_CHAIN_KEY_UNLOCK_B`. AccessibleActorList.cpp: 679, 1127.
- `NA_SE_EV_CHICKEN_CRY_M`. AccessibleActorList.cpp: 190.
- `NA_SE_EV_CHICKEN_CRY_N`. AccessibleActorList.cpp: 422, 425.
- `NA_SE_EV_COW_CRY`. AccessibleActorList.cpp: 212.
- `NA_SE_EV_COW_CRY_LV`. AccessibleActorList.cpp: 416.
- `NA_SE_EV_DIAMOND_SWITCH`. AccessibleActorList.cpp: 56, 83, 394, 535, 724, 782, 1080, 1141.
- `NA_SE_EV_DIG_UP`. AccessibleActorList.cpp: 387, 961.
- `NA_SE_EV_DROP_FALL`. AccessibleActorList.cpp: 647.
- `NA_SE_EV_ELEVATOR_MOVE`. AccessibleActorList.cpp: 474.
- `NA_SE_EV_ELEVATOR_MOVE2`. AccessibleActorList.cpp: 753.
- `NA_SE_EV_FIRE_PILLAR`. AccessibleActorList.cpp: 917.
- `NA_SE_EV_FIVE_COUNT_LUPY`. AccessibleActorList.cpp: 1096.
- `NA_SE_EV_FLUTTER_FLAG`. AccessibleActorList.cpp: 614, 657.
- `NA_SE_EV_FOOT_SWITCH`. AccessibleActorList.cpp: 61, 77.
- `NA_SE_EV_HORSE_NEIGH`. AccessibleActorList.cpp: 413.
- `NA_SE_EV_HORSE_RUN_LEVEL`. AccessibleActorList.cpp: 160, 171, 176.
- `NA_SE_EV_ICE_FREEZE`. AccessibleActorList.cpp: 941.
- `NA_SE_EV_JABJAB_HICCUP`. AccessibleActorList.cpp: 996.
- `NA_SE_EV_METALGATE_OPEN`. AccessibleActorList.cpp: 660, 747.
- `NA_SE_EV_POT_BROKEN`. AccessibleActorList.cpp: 892.
- `NA_SE_EV_RIVER_STREAM_S`. AccessibleActorList.cpp: 195.
- `NA_SE_EV_ROCK_BROKEN`. AccessibleActorList.cpp: 630, 1160.
- `NA_SE_EV_ROLL_STAND_2`. AccessibleActorList.cpp: 471.
- `NA_SE_EV_SAND_STORM`. AccessibleActorList.cpp: 202.
- `NA_SE_EV_SARIA_MELODY`. AccessibleActorList.cpp: 162.
- `NA_SE_EV_SCOOPUP_WATER`. AccessibleActorList.cpp: 620.
- `NA_SE_EV_SHIP_BELL`. ActorAccessibility.cpp: 569.
- `NA_SE_EV_SMALL_DOG_BARK`. AccessibleActorList.cpp: 178, 181, 185, 402, 406.
- `NA_SE_EV_STONE_BOUND`. AccessibleActorList.cpp: 183.
- `NA_SE_EV_TBOX_UNLOCK`. AccessibleActorList.cpp: 515.
- `NA_SE_EV_TIMETRIP_LIGHT`. AccessibleActorList.cpp: 642.
- `NA_SE_EV_TRAP_BOUND`. AccessibleActorList.cpp: 663, 788, 860, 1314.
- `NA_SE_EV_TREE_CUT`. AccessibleActorList.cpp: 435.
- `NA_SE_EV_TREE_SWING`. AccessibleActorList.cpp: 441.
- `NA_SE_EV_TRIFORCE_FLASH`. AccessibleActorList.cpp: 730.
- `NA_SE_EV_WARP_HOLE`. AccessibleActorList.cpp: 169.
- `NA_SE_EV_WATER_WALL`. AccessibleActorList.cpp: 198.
- `NA_SE_EV_WEB_BROKEN`. AccessibleActorList.cpp: 669.
- `NA_SE_EV_WIND_TRAP`. accessibility_cues.cpp: 164.
- `NA_SE_EV_WOODBOX_BREAK`. AccessibleActorList.cpp: 550.
- `NA_SE_EV_WOOD_BOUND`. accessibility_cues.cpp: 166, 282, 287.
- `NA_SE_IT_BOMB_IGNIT`. AccessibleActorList.cpp: 806, 813.
- `NA_SE_IT_BOW_FLICK`. AccessibleActorList.cpp: 450.
- `NA_SE_IT_FISHING_REEL_HIGH`. AccessibleActorList.cpp: 1008.
- `NA_SE_IT_FISHING_REEL_SLOW`. AccessibleActorList.cpp: 1006.
- `NA_SE_IT_FLAME`. AccessibleActorList.cpp: 926.
- `NA_SE_IT_HAMMER_HIT`. AccessibleActorList.cpp: 67, 640, 919.
- `NA_SE_IT_HOOKSHOT_STICK_OBJ`. AccessibleActorList.cpp: 459, ActorAccessibility.cpp: 597.
- `NA_SE_IT_KAKASHI_JUMP`. AccessibleActorList.cpp: 483, 492.
- `NA_SE_IT_REFLECTION_WOOD`. AccessibleActorList.cpp: 542.
- `NA_SE_IT_SHIELD_POSTURE`. ActorAccessibility.cpp: 540.
- `NA_SE_IT_SHIELD_REFLECT_MG`. AccessibleActorList.cpp: 774.
- `NA_SE_IT_SHIELD_REFLECT_SW`. accessibility_cues.cpp: 200.
- `NA_SE_IT_SWORD_CHARGE`. accessibility_cues.cpp: 219, 227.
- `NA_SE_IT_SWORD_IMPACT`. AccessibleActorList.cpp: 226, 317.
- `NA_SE_IT_SWORD_PICKOUT`. accessibility_cues.cpp: 245, 250.
- `NA_SE_IT_SWORD_REFLECT_MG`. AccessibleActorList.cpp: 774.
- `NA_SE_IT_WALL_HIT_SOFT`. ActorAccessibility.cpp: 537.
- `NA_SE_IT_WOODSTICK_BROKEN`. AccessibleActorList.cpp: 577.
- `NA_SE_OC_ABYSS`. AccessibleActorList.cpp: 1108.
- `NA_SE_OC_DOOR_OPEN`. AccessibleActorList.cpp: 220, 681, 690, 1129.
- `NA_SE_PL_ARROW_CHARGE_LIGHT`. AccessibleActorList.cpp: 734.
- `NA_SE_PL_CRAWL_SAND`. AccessibleActorList.cpp: 200.
- `NA_SE_PL_DAMAGE`. AccessibleActorList.cpp: 844.
- `NA_SE_PL_LAND_LADDER`. AccessibleActorList.cpp: 741, 898, 1120, 1347, 1360.
- `NA_SE_PL_LAND_WATER0`. accessibility_cues.cpp: 168, 262, 267.
- `NA_SE_PL_MAGIC_SOUL_FLASH`. accessibility_cues.cpp: 97, 104, 124, 131.
- `NA_SE_PL_PULL_UP_PLANT`. AccessibleActorList.cpp: 430.
- `NA_SE_PL_SLIP_ICE_LELEL`. AccessibleActorList.cpp: 936.
- `NA_SE_PL_SWORD_CHARGE`. AccessibleActorList.cpp: 1230.
- `NA_SE_PL_WALK_WALL`. ActorAccessibility.cpp: 543.
- `NA_SE_SY_HITPOINT_ALARM`. ActorAccessibility.cpp: 189.
- `NA_SE_SY_WARNING_COUNT_N`. ActorAccessibility.cpp: 578, accessibility_cues.cpp: 170, 303, 308.
- `NA_SE_VO_NA_HELLO_0`. AccessibleActorList.cpp: 279.
- `NA_SE_VO_NA_HELLO_1`. AccessibleActorList.cpp: 321.
- `NA_SE_VO_NB_LAUGH`. AccessibleActorList.cpp: 336, 626.
- `NA_SE_VO_RT_FALL`. AccessibleActorList.cpp: 1198.
- `NA_SE_VO_RT_LAUGH_0`. AccessibleActorList.cpp: 567.
- `NA_SE_VO_ST_DAMAGE`. AccessibleActorList.cpp: 308, 981.
