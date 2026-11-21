# Keyviz Native (C++ rewrite)

The native Windows rewrite is developed alongside the original Tauri app. The existing app remains unchanged.

## Toolchain
- Windows 10 x64
- Visual Studio 2022 (Desktop development with C++)
- Windows 10/11 SDK
- CMake 3.24+

## Local build

From a Visual Studio Developer PowerShell:

```powershell
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
- Monitor-aware placement inside each display's work area
- A single hidden top-level input window owns keyboard and mouse Raw Input registration
- Input events are shared through one event engine and redrawn on every monitor overlay
- Display topology changes trigger overlay teardown and recreation
- Normalized key press/release with tracked left/right modifier state, foreground keyboard-layout translation including E0-prefixed extended scan codes and visible dead-key accents, left/right/middle/X1/X2 mouse buttons, vertical and horizontal wheel events
- Mouse movement sampling with unified virtual-desktop cursor coordinates; samples are coalesced in history
- Extracted InputEvent, FilterEngine, HistoryStore and EventEngine modules
- CTest coverage for history capacity limits, runtime resizing, clearing, event coalescing, persistence round-trips, left/right Ctrl normalization, modifier chords and AltGr state
- Bounded event history with configurable 40/80/160/320-event retention
- Versioned history file in `%LOCALAPPDATA%\\Keyviz\\history.bin`, atomically replaced and checkpointed every 30 seconds and on clean shutdown
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
- Pointer coordinates are virtual-desktop screen coordinates; per-monitor local coordinate presentation is not yet configurable.
- The Direct2D HWND target uses global layered-window opacity, not per-pixel transparency.

## Next milestones
1. Validate dead-key and keyboard-layout behavior on physical Windows systems; expand animation styles.
2. Investigate a separate, opt-in text-input integration for IME-committed text and richer history navigation.
3. Complete physical-device validation.

This is an incremental development build, not yet a complete Keyviz replacement.
