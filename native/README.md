# Keyviz Native (C++ rewrite)

The native Windows rewrite is developed alongside the original Tauri app. The existing app remains unchanged.

## Keyboard device state

Keyboard modifier state is tracked per Raw Input device. Only keyboard input packets create state entries, so a removal notification directly erases the matching device handle without querying metadata that Windows may already have invalidated. Unknown removals leave other keyboards untouched.

## Toolchain
- Windows 10 x64
- Visual Studio 2022 (Desktop development with C++)
- Windows 10/11 SDK
- CMake 3.24+

## Local build

From a Visual Studio Developer PowerShell:

```powershell
./native/resources/generate-icon.ps1
cmake -S native -B build/native -A x64
cmake --build build/native --config Release
ctest --test-dir build/native -C Release --output-on-failure
```

Output: `build/native/Release/KeyvizNative.exe`.

## Windows installer

The repository includes an Inno Setup definition at `installer/keyviz-native.iss`. On a Windows x64 build machine with Inno Setup 6 installed, generate the branded multi-size application icon, build the native executable, then run:

```powershell
& "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe" installer/keyviz-native.iss
```

The per-user installer is written to `build/installer/KeyvizNative-Setup-x64.exe`. It installs under `%LOCALAPPDATA%\\Programs\\Keyviz Native`, adds a Start Menu shortcut, optionally creates a desktop shortcut, and offers to launch the app after setup. It does not configure automatic startup. The uninstaller is registered through Windows Apps settings.


## Current implementation
- C++20 Win32 GUI application with a branded multi-size executable/tray icon, embedded Windows product/file version metadata (0.2.0) and per-monitor DPI awareness V2 request
- One click-through, topmost overlay window per enumerated display
- Per-monitor effective-DPI-aware placement with consistent 580×156 DIPs and 36-DIP bottom margin across mixed-scale displays
- Mixed-DPI geometry regression tests at 100%, 150% and 200% scaling
- A single hidden top-level input window owns keyboard and mouse Raw Input registration with device-change notifications; removal of a keyboard resets tracked modifiers to prevent stuck chords
- Input events are shared through one event engine and redrawn on every monitor overlay
- Display resolution changes and device-tree notifications schedule a debounced (300 ms) overlay rebuild, covering monitor hot-plug and topology changes
- Normalized key press/release with tracked left/right modifier state, foreground keyboard-layout translation including E0-prefixed extended scan codes and visible dead-key accents, left/right/middle/X1/X2 mouse buttons, vertical and horizontal wheel events (zero-delta packets ignored)
- Mouse movement sampling capped at 20 Hz, with consecutive movement samples coalesced within each uninterrupted movement run; intervening button and wheel events remain in order and are never overwritten; button and wheel events carry the cursor position captured with that Raw Input packet, so the overlay refreshes pointer coordinates on clicks and scrolling as well as sampled movement; unified virtual-desktop cursor coordinates are used and the overlay labels desktop-absolute coordinates and active-monitor-local coordinates separately, including negative origins on left/upper displays
- Extracted InputEvent, FilterEngine, HistoryStore and EventEngine modules
- CTest coverage for history capacity limits, runtime resizing, clearing, truncated-file and trailing-data rejection without replacing live history, future wall-clock timestamp rebasing, failed atomic replacement preserving the existing target and cleaning temporary files, movement coalescing across action boundaries (including monotonic and wall-clock timestamp refresh), persistence round-trips, per-device repeat-history isolation for interleaved same-key presses across keyboards, left/right Ctrl normalization, right Shift/right Alt chord tracking, simultaneous left/right modifier holds and side-specific release behavior, modifier-state reset after keyboard removal, independent per-device key-repeat detection, shared modifier aggregation and side-specific release across multiple keyboards, capture stop/restart state reset across multiple keyboards, printable/dead-key translation-label formatting and fallback behavior, modifier chords, AltGr state, mixed-DPI overlay geometry, virtual-desktop pointer coordinate formatting, mouse sampling cadence/tick-wraparound, and signed vertical/horizontal wheel direction including zero-delta rejection
- Bounded event history with configurable 40/80/160/320-event retention
- Versioned history file in `%LOCALAPPDATA%\\Keyviz\\history.bin`, atomically replaced using process-scoped temporary files with checked flush/close and failed-save cleanup, and checkpointed every 30 seconds and on clean shutdown
- Persisted wall-clock timestamps are rebased to the current process tick count when loaded, keeping history ages and overlay expiry valid across restarts
- History retention and overlay fade duration (0.4/0.8/1.1/1.6 seconds) persisted in `%LOCALAPPDATA%\Keyviz\settings.ini`
- Notification-area tray icon with Settings, Input history and Exit commands
- Inno Setup per-user x64 installer with Start Menu and optional desktop shortcuts
- Native Win32 settings window; Ctrl+Shift+K shortcut
- Native input history window; Ctrl+Shift+H shortcut
- History search with case-insensitive key/chord matching and event-type filters
- History result count and explicit empty/no-match states
- UTF-8 CSV export of the current filtered history view
- Independent Direct2D HWND render target and DirectWrite renderer per display
- Event-driven 16 ms redraw timer with a short fade-out window

## Known limitations
- Runtime behavior still needs testing on physical multi-monitor Windows 10 systems, including mixed-DPI and hot-plug configurations.
- Overlay fade timing is configurable; richer animation styles and easing remain future work.
- A sudden power loss or forced process termination can lose events recorded since the last 30-second history checkpoint.
- Global IME composition and committed-text capture are intentionally not implemented; see `native/docs/IME_AND_TEXT_INPUT.md` for the architecture and privacy assessment. Dead-key accents are labeled, but locale-specific sequences still need physical-device validation.
- Pointer coordinates are shown as both virtual-desktop screen coordinates and coordinates relative to the active monitor's top-left corner; monitor-local coordinates are display-only and do not replace stored virtual-desktop values.
- The Direct2D HWND target uses global layered-window opacity, not per-pixel transparency.

## Next milestones
1. Validate dead-key and keyboard-layout behavior on physical Windows systems; expand animation styles.
2. Investigate a separate, opt-in text-input integration for IME-committed text and richer history navigation.
3. Complete physical-device validation.

This is an incremental development build, not yet a complete Keyviz replacement.
