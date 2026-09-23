# Fresh-install extraction crash

Amethyst reported that the fresh copy at `C:/Users/Amethyst/Documents/ocarina` crashed
after choosing a supported ROM. Its executable, port archive, Prism and Steam Audio DLLs
match the prepared release package for build `d4342490da07`. No generated `oot` archive was present.
The current fixed build and manual test are recorded in [todo.md](../../todo.md).

## Crash evidence

Windows Application Error events record four matching crashes on 22 September, between
02:07 and 02:09 local time. They are access violations (`0xc0000005`) inside `soh.exe`, at
RVA `0x147e1e0`, reading address zero. The corresponding minidump
`C:/Users/Amethyst/AppData/Local/CrashDumps/soh.exe.24428.dmp` contains the exception context:

- Executable base: `0x7ff66d210000`.
- RIP: `0x7ff66e68e1e0`; RCX, the member-call receiver: zero.
- RSP: `0x47454d0a8`; the immediate return address is `soh+0x1481aee`.
- The matching PDB SHA-256 is `fd8a23f115e8aeccda9c9a3d019331625d3897d7193140a354dc5a5c788c4e87`.

LLVM symbolization with that PDB maps the leaf instruction to
`Fast::GfxDebugger::IsDebugging`, `libultraship/src/fast/debug/GfxDebugger.cpp:27`.
Identical-code folding also assigns the public name `_zip_buffer_ok` to this tiny function;
the actual immediate caller is `LUS::GfxDebuggerWindow::UpdateElement`, line 23. The
member receiver is null, and that caller unconditionally reads `GetGfxDebugger()->IsDebugging()`.

Other code-address candidates on the saved stack map to `Ship::Gui::DrawMenu`,
`Ship::Gui::StartDraw`, and `OTRGlobals::RunExtract` at its first `gui->StartDraw()` call.
The stack scan is not claimed as a complete unwind. Its immediate return address, null
receiver, matching source line and verified startup lifecycle identify the fault.

These addresses describe this particular crashed binary only. No runtime offsets, hooks
or RE-database annotations were added. The helper `analyze_dump.py` is retained under
`C:/Users/Amethyst/source/ocarina-build/setup-crash-20260922`. The original dump is unchanged.

## Source lifetime and fix

`InitOTR` constructs `OTRGlobals`, initializes Prism/text banks, calls `RunExtract`, and
only then calls `OTRGlobals::Initialize`. The constructor creates the rendering window
and its GUI. `Ship::Gui` automatically creates its debugger window; `Gui::DrawMenu`
updates all GUI windows even when their panels are hidden.

The native-menu adaptation made the debugger window's update query the debugger state so
it can open the native debugger page on a break. The service itself was still initialized
only in `OTRGlobals::Initialize`, after extraction. Selecting a ROM schedules extraction
and enters the GUI render loop, exposing that missing service. An already-extracted
installation skips this render loop and reaches the later initializer first.

Move the existing `InitGfxDebugger()` call into the constructor, before `InitWindow()`.
Remove the later call. The actual initializer simply creates `Fast::GfxDebugger`, whose
flags and display-list pointer start false/null; it needs neither a ROM, audio device nor
a game state. The debugger window and native overlay can therefore query it during setup.
This fixes their shared dependency, without suppressing debugger callbacks or changing
speech, the file picker, extraction policy or the user's configuration.

The missing log folder is also consistent with this path: normal file logging starts in
`OTRGlobals::Initialize`, which the crash never reached. Prism is not on the failing call
path. Normal game audio initialization happens after `RunExtract` as well.

## Verification and limits

The Windows Release build passes. Both existing Native Options suites and the speech suite
pass. The corrected source places debugger creation before window construction and before
all extraction rendering; the source was checked against the actual dump and callback.
Other early native-menu input handling already returns while its model is uninitialized.

The game was not launched, relaunched, closed or controlled by Codex. A complete extraction
with the new build still requires Amethyst's test. Package CRCs, runtime hashes, replacement
backups and the two installed copies are recorded in the deployment manifest and checklist.

Corrected build `3070a31e1701` is installed in both the Documents copy and
`H:/projects/ocarina/game`. Only the executable and matching symbols were replaced. Their
previous files and the root checklist/installation guide are preserved under
`H:/projects/ocarina/backups/9.2.3-before-setup-fix-20260922`. Game data and settings were
hash-verified unchanged. The corrected distribution is
`_packages/Ship-9.2.3-setup-fix-prism-win64-ship.zip`; full hashes are in
[the build notes](accessibility-build.md) and the local
`setup-crash-20260922/setup-fix-deployment-manifest.json` record.
