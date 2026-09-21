# Native Options migration

## Status

The migration is implemented. Amethyst confirmed the title crash fix and menu feedback update. The remaining file-select return announcement is fixed, built, packaged and deployed as 66a8f25eff60. Artifact hashes and current manual checks are in accessibility-build.md and the root todo.md. Codex did not launch or control the game; the focused return test remains with Amethyst.

Amethyst approved the blue panel, gold focus, categories and Advanced tools. Options now opens from title, file selection, Escape and a fifth pause tab between Equipment and Items. Her latest answers select game bindings for navigation, release-gated remapping with Escape/ten-second cancellation, and the fifth pause tab. There are no unanswered design questions.

The previous uncommitted source, including deleted files in a binary Git patch, is preserved in H:\projects\ocarina\backups\9.2.3-before-native-options-20260921\source-before-native-options.zip.

## Title Options crash, 21 September

Amethyst reported that selecting Options on the title screen crashes build 7dfd12a63b9b. The installed log records
`Graph_Update` at graph.c:365 followed by `Fault_AddHungupAndCrash`; that branch checks `GfxPool::tailMagic`.
The final access violation is the game's deliberate fault-handler write, after graphics memory was already damaged.

`GfxPool::overlayBuffer` has 0x800 (2,048) Gfx entries. The original NativeOptions renderer appended every glyph's
texture load and rectangle directly to it. Each textured primitive emits 13 Gfx entries, including its state commands;
text shadows double the glyphs. The standalone root regression layout emits 4,609 commands and a dense details page
emits 19,755. The earlier Pillow previews exercised layout geometry, not this GBI emission or its buffer capacity.

The fix moves the actual GBI emitter into OptionsDisplayList.cpp. Each primitive uses a bounds-checked command packet;
completed packets append to an owned, growable display list. OptionsRenderer only appends setup and a list call to
the game's overlay. Geometry, colors, texture operations, navigation and speech are unchanged.

DisplayLists retains separate storage for both graphics-pool slots and each menu drawn in one frame. Graph_InitTHGA
resets the corresponding slot's usage when that pool is reset. Graph_Update skips that reset during debugger capture.
RunFrame consumes the lists in Graph_ProcessGfxCommands after Graph_Update; RunCommands completes all interpolation
passes synchronously. This preserves lists for title/pause/full-menu draws, interpolated frames and paused captures.
The native list link uses __gSPDisplayList: unlike the game's resource-name wrapper in GbiWrap.cpp, its target is
already owned Gfx memory. Every owned list terminates with G_ENDDL to return to the overlay.

The standalone tests now execute the production GBI emitter. They cover the root and dense detail overflow cases,
guarded overlay links, same-frame list growth, alternating pool slots, retained capture data, glyph opcode order,
resource pointers, both image formats, GUI-only images and backdrop clipping. Both CTest suites and the Windows x64 Release build passed. The verified package is deployed as 5c92318f6999.
The previous executable, symbols and crash log are preserved in H:\projects\ocarina\backups\9.2.3-before-options-crash-fix-20260921. Codex did not launch or control the game for that fix. Amethyst subsequently confirmed that Options opens from the title screen successfully.

## Native feedback and window state, 21 September

Amethyst confirmed that build 5c92318f6999 opens Options from the title screen successfully. She reported missing
confirm/back sounds, no value for Toggle Fullscreen, and an unexpected Options opening during startup.

- OptionsRuntime previously played only the controller cursor sound. Model::Navigate now handles controller and
  keyboard actions through the same feedback callback. The runtime uses the game's existing file-select sounds:
  NA_SE_SY_FSEL_CURSOR, NA_SE_SY_FSEL_DECIDE_L, NA_SE_SY_FSEL_CLOSE and NA_SE_SY_FSEL_ERROR. FileChoose's confirmation
  and cancellation consumers establish these mappings. Programmatic Push/Back/Refresh stay silent, so accepting a
  choice that returns to its parent does not produce both confirm and back sounds. Root opening/closing, details,
  capture completion/cancellation and returning from the pause Options tab have explicit native cues.
- SohMenuSettings registered Toggle Fullscreen as WIDGET_BUTTON with a ToggleFullscreen callback. It was an immediate
  A action, not a submenu or an adjustable setting, and its Row had no value. It now uses the existing WIDGET_CHECKBOX
  adapter: PreFunc reads Window::IsFullscreen, the value pointer carries that live snapshot, and Callback calls
  Window::SetFullscreen with the selected value. The label and description remain the registry's exact captions;
  On/Off comes from the central Options text bank. Model refresh reads the actual window state after A or left/right,
  and after external changes such as F11. DXGI and SDL SetFullscreen update the backend state synchronously.
- GuiWindow's constructor restored its visibility CVar, and Show/Hide persisted it. SetupMenu previously used
  CVAR_WINDOW("Menu"), which could restore an open Options screen following exit or a crash. The menu now uses an
  empty visibility CVar, the library's supported nonpersistent mode, and clears the obsolete saved flag during setup.
  Explicit entry points and required native popups remain available. The current config no longer contained the flag
  when inspected after Amethyst's clean exit, so the exact state at the earlier startup was not captured.

Offline navigation tests cover confirm/back events, single feedback for child acceptance, unavailable and descriptive
rows, root closure, initial and changed toggle values, external state changes and preserved speech ordering. Both
CTest suites and the Windows Release build passed, including the actual GBI overflow regression. Build daf31abcf6b7 is deployed; the previous executable and symbols are preserved in H:\projects\ocarina\backups\9.2.3-before-options-feedback-20260921. Amethyst subsequently confirmed the update works, with the file-select return issue described in accessibility-speech.md. Codex has not launched or controlled the game.

## Implementation checkpoint

- The foundation, audio, input integration, title, details reader, cosmetics, randomizer, plandomizer, trackers, splits, gameplay statistics and Advanced tools have passed Release builds. The controller editor and pause entry are now connected and have passed Release builds. In-game verification remains with Amethyst.
- Standalone previews were rendered from the production layout and the installed archive's actual I4 font and IA16 panel textures. Audio and long-description previews were visually inspected in 4:3 and widescreen. Artifacts are under C:\Users\Amethyst\source\ocarina-build\native-options-preview. The preview runner does not start the game.
- ResolutionEditor.cpp now defines a native page; its old ImGui settings body is deleted. It preserves aspect ratio, fixed pixel counts, horizontal editing and integer scaling. The old maximum-scale calculation accidentally shadowed its state variables; the native page calculates bounds directly from current renderer dimensions.
- Presets.cpp now defines native creation, section selection, apply and delete pages. Tracker overlay geometry is still captured through the existing renderer. Its ImGui settings controls are deleted.
- mod_menu.cpp now defines native mod ordering, cancel, clear and apply/close pages. The old ModMenuWindow is removed. InitializeMods is called at the original initialization point. B discards the pending order.
- AudioEditor.cpp now defines native sequence groups, replacements, previews, lock/reset/randomization and shuffle-pool management. Its window is removed; InitializeAudioEditor preserves all three automatic-randomization hooks. The old regular widgets still supply their real captions, limits and defaults.
- OptionsNetwork.cpp defines Sail/Crowd Control host/port pages and Anchor connection, room, admin and instruction pages, registered from NativeOptions::Init. The original network custom renderer bodies are removed.
- InputViewer settings now have native pages; the display overlay remains. Its separate settings window and registration are removed.
- GameState_Update gates state main and game-frame hooks while native Options renders its own full frame. Keyboard commands queue from the existing platform input backend, including UTF-8 typing and clipboard paste. The gamepad uses mapped game input. libultraship's GuiWindow has explicit native gamepad and keyboard-capture queries; ControlDeck respects them. Closing clears pad input and suppresses held input until released. R or F1 opens the selected item's full, paged description. These changes still need manual verification.
- SaveManager separates LoadGlobal/EnsureGlobalLoaded from file metadata initialization. Native Options loads original global preferences before editing, including during boot logos. Original audio/target rows use the existing localized file-selection resources, actual FS enum values and SaveGlobal. Audio changes call func_800F6700.
- z_en_mag adds two title choices using the existing localized start caption and Options. The title artwork keeps its once-per-opening announcement; rows include positions and queue initial hints. FileChoose's existing Options action opens the shared native menu.
- OptionsControllerDevices/OptionsControllerCamera preserve device, stick response, rumble, LED, gyro, aiming, camera and D-pad settings. These factories now connect through OptionsControllerRegistry.cpp and the approved OptionsControllerCapture.cpp lifecycle.
- CosmeticsEditorWindow and its ImGui rendering are deleted. InitializeCosmeticsEditor preserves initialization; native pages retain color groups, side effects, rainbow/locks, bulk actions, dungeon key colors, extra effects, margins and HUD placement. Enemy health-bar offsets now write the single-dot keys consumed by z_parameter.c:3631, rather than the previous double-dot keys. Opening C-button placement no longer performs unrelated ocarina remapping. Bulk margins now obey the documented exclusion of hidden/unanchored elements.
- SohMenuRandomizer's seed/spoiler, location and trick rendering is replaced with native pages. Names/descriptions come from the real settings/location tables. Included/excluded and enabled/disabled lists retain area grouping, filters, tags and bulk actions. Area filters replace the old expand/collapse state used to select visible bulk targets. Generation thread joining is independent of whether the old Spoiler File widget draws. Randomizer numeric option lists now expose their real captions as native choices, not editable internal indices.
- MenuTypes.h now supports native page factories. Registered window switches with EmbedWindow(false) remain overlay toggles rather than opening a configuration page.

Packaging, backup, deployment and the manual checklist are complete. In-game checks remain with Amethyst.

- Plandomizer now has native hash icons, hint editing, location rewards, prices, ice traps and finite-pool controls. Loads validate before replacing current state. Saves preserve unknown JSON fields, refuse external file changes and keep a unique backup.
- Tracker settings, notes and interactive check/entrance browsers use native pages. Live item/check/entrance overlays remain; their former settings controls are removed.
- Time Splits has native split creation, variants, ordering, skip/remove, named lists, attempt reset and appearance. Its remaining overlay is a read-only live table. Named-list saves use the shared backup-and-replace helper.
- Gameplay Stats now has native timers, timestamps, counts, scene/room breakdown and options. Its old window is removed.
- Collision, hook, value, message and performance viewers have native pages. Console migration preserves its underlying command backend, command help/history, channel/level filtering and logs. Removed UI callbacks are not kept as alternate settings views.
- Offline checks also cover multiline text, result dialogs after editor commits, speech cancellation on close, and file replacement/backup preservation. These and the Advanced Release build passed.
- Console, Warp Points, graphics debugger, display-list editor and actor editor have now passed Release builds. Actor parameters are in actorParameters.cpp; enum captions were moved from the old editor into central Options resources. The old editor windows and registrations are removed.
- The console remains a backend service for command bindings and log sinks. Its ImGui input/tree/list widgets are deleted. It exposes synchronized log snapshots and native opening callbacks. ClearLogs now clears the stored channels, and string messages use a literal format argument so percent signs are safe.
- The graphics debugger retains its captured display-list buffer. Graph_Update now polls pad/native-menu input while capture is active and leaves the buffer intact. A generic DrawGameOverlay callback draws the same production native layout, font and panel through the existing ImGui renderer over the captured frame. Its command tree, breakpoint selection, copy actions, display stack, texture previews and colors have native pages. This special rendering/input path still needs in-game verification.
- Actor selection is invalidated by native actor-destroy and scene-init hooks. Field edits verify that the selected actor is still current. Native parameter controls preserve unrelated bits; Wolfos reads its full high byte as the switch flag (z_en_wf.c:226), filepath vertex commands read their count/offset from their second word (interpreter.cpp:3099), and replacing multiword display-list commands clears their trailing words to pipe syncs.
- Fourteen standalone visual previews now cover lists, confirmations, icons, full details and texture previews in 4:3 and widescreen. The audio list, seed icons, texture preview and widescreen details were inspected. Native navigation/editor/file checks passed again after these changes.
- Plandomizer and split-list file factories now expose listing failures as rows instead of pushing a popup during page construction. Split-list numeric fields reject negative, fractional and overflowing values before conversion; file-owned resource names/colors are replaced by the current verified split definition.

## Verified architecture

- The complete native save editor, modal queue, frame-advance controls and settings search passed the Release build in build-native-search.log. Save fields use their real widths, including u16 dayTime and full-range unsigned flags. Randomizer flag tables use actual array bounds instead of the old inclusive size loop. Live player/scene callbacks use pointer and scene-generation guards. The old save editor and modal windows are removed.
- Modal requests retain their source captions and callbacks and open through the native update path. File-selection links now request the actual native Presets and Randomizer pages. F1's obsolete “Menu Moved” popup is removed; F1 is the native full-description action. Console bindings, Ctrl+R and game save-state hotkeys are blocked while Options owns keyboard input.
- FrameAdvanceContext.enabled is an s32 (z64.h), not a bool. The native toggle writes it directly. GameState_Update may run an explicitly requested frame while the menu stays open, with pad input cleared; normal menu use remains paused. Holding A or Enter on Advance (Hold) repeats through a row-held callback. Manual verification remains required.
- The shared Options text preparation translates the exact Font Awesome arrow, sign and Menu glyphs produced by SDLButtonToAnyMapping and SDLAxisDirectionToAnyMapping. It removes the decorative warning icon and preserves punctuation/newlines. Offline tests for these source glyphs, unsigned limits and stale write guards passed with the existing navigation/editor checks.
- Native search filters real registry rows by caption, value, description, category and registered extra terms. It preserves the setting callbacks and disable rules. Embedded editor links no longer describe a removed popout window.
- The check and entrance tracker browsers passed Release builds. Their search, filter, expansion and check-skip controls now live in native pages; the old overlay buttons and trees are removed. Entrance visibility preserves the original discovery, reverse-entry, grouping and mystery rules. Check changes still call the tracker ordering, inventory and save-section consumers.
- Menu cleanup, overlay geometry, title layout and controller binding page factories passed the Release build in build-native-menu-cleanup.log. The controller capture API changes passed build-native-capture-api.log. The controller page factories are now connected using the three approved answers.
- Native overlay layout reads the live ImGui window or its existing saved window settings. ImGui stores positions relative to the owning viewport; the menu reports positions relative to the game viewport. Writes preserve docking unless explicitly undocking, and use the existing settings persistence. Preset loads now update this same state immediately, and preset capture includes saved windows not opened in this session. Bean Soul and Jabber Nut windows were missing from the old preset window-ID list and are included now. No new parallel layout configuration is introduced.
- Offline tests use the exact game-build ImGui source with a headless context. They passed for unopened settings, nonzero viewport origins, explicit undocking, invalid geometry rejection, live window edits and serialization/reload. Both native-options-navigation and native-options-overlays passed. The resource-key audit and both Git whitespace checks also passed.
- Eighteen standalone images now include title Start/Options selections, categories, audio, long descriptions, seed icons, texture previews, confirmations and details at 4:3 and widescreen. The title, category, widescreen audio and confirmation images were inspected after the latest layout update. The title preview places the real logo/copyright assets using EnMag_DrawInner's coordinates, without simulating its animated world or flame effects. Production and preview title rendering now share TitleLayout.
- Deleted the old main menu's header/sidebar/search/popout renderer and obsolete theme, opacity, popout and sidebar-search controls. The legacy MenuDrawItem adapter, unused interactive UIWidgets helpers, and both old controller editor windows and the duplicate libultraship performance window are now deleted. Removed unused Option::RenderCheckbox/RenderCombobox/RenderSlider functions. Registry extras are search entries only, avoiding duplicate specialist settings in category pages. Overlay scaling is retained under Trackers because ScaleImGui also changes HUD text and spacing.
- CanWrite re-evaluates widget hidden/disable/race state before committing numeric, choice, checkbox, button-combination and color changes from child editors. Color writes also re-check the live lock. The full build passed after this guard was added.
- ControllerButton and ControllerStick now expose CancelMappingCapture; Controller clears capture across its buttons and both sticks. Raw remapping accepts an explicit mouse-capture flag, removing the native editor's dependence on IsMouseOverActivePopup. Event processing still reaches the controller objects while mapped keyboard gameplay input is blocked. The capture runtime clears state on entry, release arming, completion and every cancellation/close path.
- Anchor incoming packets continue to drain while native Options pauses gameplay, matching the old OnGameFrameUpdate behavior during pause. Graphics debugger capture retains its frozen-packet behavior. Native global-room details only expose the old online count. Message previews validate the table/text/custom-message buffer before closing Options.
- Startup extraction keeps desktop native dialogs. Console-platform setup popups retain an assets-independent renderer used only by RunExtract, because game fonts/panels do not exist before ROM extraction. In-game popups use the new native menu. Setup popups and timer layout passed build-native-setup-timers.log. Console-platform execution has not been tested.
- The coverage follow-up passed build-native-coverage.log. Search results now include specialist locations. Audio/video backend controls preserve their original explanations and single-backend disable reasons, and revalidate availability before committing. Overlay switches and native option choices also revalidate writes. Removed the unused trick-tag ImGui renderer and obsolete SohMenu state.
- Saved split lists refresh after creation. Token split events now check each active split's own token target, instead of permitting any token target elsewhere in the list to authorize completion. z_parameter.c increments gsTokens before Return_Item dispatches OnItemReceive, so the comparison uses the new total. The migrated list editor exposed this existing defect; in-game verification remains required.
- Category review moves the registry's Controls and Camera Fixes sections into Controls, Audio Fixes into Audio, and Graphical Fixes/Restorations into Display. Background input and reset bindings also belong under Controls. These changes passed the Release build in build-native-category-review.log.

## Controller and pause integration

- OptionsControllerRegistry.cpp owns the seven existing control widget definitions/search entries and registers Configure Controller. OTRControllerCallback uses TestingControllerRumble instead of the obsolete window lookup.
- Capture uses Wait, Arm, Poll and Cancel states. Arming clears stale presses and polls on the neutral frame, enabling event capture before the next press. Keyboard/mouse presses latch until consumed or cancelled; releases still reach existing mappings. This preserves a quick down/up in one frame. Cleanup covers all four ports and both sticks. Editing returns to the mapping list because the mapping ID can change.
- Escape is consumed during capture, leaving Options open. Legacy ImGui controller navigation and its physical Back shortcut cannot override the approved game bindings. Keyboard arrows, Enter, Backspace and F1 remain available to restore cleared controller mappings.
- PauseContext::optionsTab is separate from pageIndex. PAUSE_ITEM through PAUSE_EQUIP remain 0 through 3. PAUSE_WORLD_MAP (4) remains a cursor-array index, never a fifth page index. OptionsPauseNavigation.h inserts Options at Item-left and Equipment-right. Reversing restores the retained page; continuing uses the original wrap rotation. B returns to the retained page, A opens settings, Start uses the original unpause flow.
- The tab uses the production panel/font layout. Adjacent native arrow captions reuse the 64-by-16 IA16 Options artwork in title_static, with its original language mapping. Play_DrawOverlayElements and the gameplay timer omit HUD drawing over this tab. SohMenu::HidesGameOverlays suppresses live overlays while either native Options surface is visible.
- Pause TTS uses a speech-only sentinel for Options, never an array index. It reads Options, Open Options with its description and 1 of 1, then the displayed tab hints. Closing settings reintroduces the retained pause page.
- Time Splits column visibility/order moved into Window Options, Columns. A toggles visibility; left/right reorder; at least one column must remain. OptionsTableLayout.h reads/writes the existing ImGui table settings, preserving saved layout and other column data. Plain captions replace the interactive headers. Headless tests verify the actual BeginChild/GetID identity, unopened/live edits, reload, invalid orders and all-hidden rejection.
- Final offline checks cover navigation, delayed queues, parent returns, popup positions, values, UTF-8 editing, file backups, font glyphs, capture cancellation/timeout/clock rollover, pause routes, overlay geometry and table persistence. Twenty-two production-layout previews cover title, categories, audio, descriptions, icons, texture previews, confirmations, details, remapping and pause at 4:3 and widescreen. These are layout previews, not game tests.

## Original source findings

- libultraship/src/ship/window/gui/Gui.cpp handles Escape in Gui::DrawMenu and toggles the registered GuiWindow menu. SohGui::SetupMenu installs SohMenu there. The same library uses ImGui for the main game viewport, so removing the settings UI is distinct from removing the renderer's ImGui dependency.
- soh/soh/SohGui/SohMenu.cpp registers five top-level menus. MenuInit adds further definitions from controller, audio, cosmetics, resolution, presets, mods, randomizer trackers, enemy randomizer and Anchor networking code.
- The original MenuTypes.h defined WidgetInfo with the actual caption, CVar, option defaults/limits, callback, preFunc, postFunc, hidden/disabled state and window/custom renderer. These definitions can feed a native view while preserving setting semantics. Main menu call-site counts alone are not a complete setting inventory: randomizer OptionGroup::AddWidgets and several editors create entries dynamically.
- The removed Menu::MenuDrawItem invoked preFunc before rendering, assembles disable reasons and race lockout, then calls the change callback only after a user change. It called postFunc after drawing; native held actions now have explicit row callbacks. Native controls must preserve those distinctions; calling rendering lambdas as actions would be incorrect.
- Some callbacks call ImGui directly, such as copying a network URL. Custom widgets and embedded windows require individual migration, not a generic invocation of the old renderer.
- FileChoose_ConfigModeDraw draws the original textured blue panel and calls PAL or NTSC Options rendering during CM_MAIN_TO_OPTIONS through CM_OPTIONS_TO_MAIN. Options opens from the file selection menu, not gameplay.
- The original FileChoose_UpdateOptionsMenu edited gSaveContext.audioSetting and zTargetSetting directly; PAL N64 can also edit language. Returning saves the global save header and CVars, then applies func_800F6700. Those original settings must survive the migration.
- Interface_DrawTextLine uses Ship_GetCharFontTexture and Ship_GetCharFontWidth, with a 16-by-16 I4 character texture, R_TEXT_CHAR_SCALE and optional shadow. Its local textLength is an unsigned byte, so a new long-text layout must not pass arbitrarily long strings to it.
- GameState_Update calls gameState->main, which combines state updating and drawing. Freezing a native Options screen cannot simply skip that call without providing a rendered frame. The gameplay update/draw split must be traced before adding a pause gate.
- ControlDeck::GamepadGameInputBlocked blocks controller input when the ImGui menu is visible and its controller-navigation CVar is on. Native navigation must explicitly handle that existing gate and avoid forwarding menu input into gameplay. Keyboard and mouse blockers have different conditions.

## Migration work

1. Complete the inventory, including dynamic controls, callbacks, disable reasons and persistent configuration.
2. Resolve the design questions and record the approved taxonomy and navigation.
3. Introduce a native settings model and game renderer, with one source for visible and spoken captions, persistent focus and screen lifetimes.
4. Connect file-selection Options and the approved gameplay entry point; retain the original audio/target settings.
5. Migrate registered widgets and specialized editors. Preserve live HUD overlays separately from their configuration screens.
6. Remove replaced UI implementations and registrations after each replacement is complete; do not leave dead buttons or inaccessible fallback windows.
7. Verify navigation, persistence, conditional settings, controller capture, speech order and positions. Build and inspect standalone render previews without launching the game.
8. Build and package Release, preserve the installed Prism build, deploy, and refresh todo.md for Amethyst's manual test.

## Specialist surfaces found

- Controller bindings: buttons, two sticks, device toggles/defaults, rumble, LED and gyro mappings, camera, ocarina and D-pad behavior.
- Display: renderer, output/resolution, aspect ratio, integer scaling and runtime-dependent limits.
- Audio: registered audio settings, sequence replacement, preview, lock, randomize and reset controls.
- Cosmetics: color groups, individual channels, locks, rainbow behavior, bulk actions, HUD placements and additional effects.
- Randomizer: dynamically generated OptionGroups, seed/spoiler input, excluded locations, tricks/glitches and plandomizer.
- Trackers: item, entrance and check settings; gameplay stats, time splits and timers. Their display overlays are not equivalent to their configuration UI.
- System: presets, mod archives, notifications and application actions.
- Network: Sail, Crowd Control and Anchor, including host/port inputs and connection/room state.
- Developer tools: debug options, warp points, console, save editor, hook/actor/collision/value/message/display-list/graphics viewers and performance stats. Amethyst approved preserving these under Advanced.
