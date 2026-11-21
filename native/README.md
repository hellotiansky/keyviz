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
```

Output: `build/native/Release/KeyvizNative.exe`.

## Current implementation
- C++20 Win32 GUI application and per-monitor DPI awareness V2 request
- One click-through, topmost overlay window per enumerated display
- Monitor-aware placement inside each display's work area
- A single hidden top-level input window owns keyboard and mouse Raw Input registration
- Input events are shared through one event engine and redrawn on every monitor overlay
- Display topology changes trigger overlay teardown and recreation
- Normalized key press/release, left/right/middle/X1/X2 mouse buttons, vertical and horizontal wheel events
- Mouse movement sampling with unified virtual-desktop cursor coordinates; samples are coalesced in history
- Extracted InputEvent, FilterEngine, HistoryStore and EventEngine modules
- Bounded in-memory event history
- Native Win32 settings window; Ctrl+Shift+K shortcut
- Native input history window; Ctrl+Shift+H shortcut
- History search with case-insensitive key/chord matching and event-type filters
- History result count and explicit empty/no-match states
- UTF-8 CSV export of the current filtered history view
- Independent Direct2D HWND render target and DirectWrite renderer per display
- Event-driven 16 ms redraw timer with a short fade-out window

## Known limitations
- Runtime behavior still needs testing on physical multi-monitor Windows 10 systems, including mixed-DPI and hot-plug configurations.
- Animation is a basic event fade, not yet configurable.
- Settings persistence and filter editor are implemented; history search/filtering is available, but long-term history persistence is not implemented.
- Full keyboard layout/IME and modifier-state semantics remain to be implemented.
- Pointer coordinates are virtual-desktop screen coordinates; per-monitor local coordinate presentation is not yet configurable.
- The Direct2D HWND target uses global layered-window opacity, not per-pixel transparency.

## Next milestones
1. Add robust modifier/keyboard-layout handling and configurable animation.
2. Add configurable history retention and richer history navigation.
3. Add tray integration and installer packaging.

This is an incremental development build, not yet a complete Keyviz replacement.
