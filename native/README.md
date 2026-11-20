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
- Transparent, topmost, click-through overlay
- Background keyboard and mouse Raw Input in a dedicated `InputCore` module
- Normalized key press/release, left/right/middle/X1/X2 mouse buttons, vertical and horizontal wheel events
- Compound mouse-button packets expanded into individual events
- Extracted `InputEvent`, `FilterEngine`, `HistoryStore` and `EventEngine` modules
- Bounded in-memory event history
- Native Win32 settings window; Ctrl+Shift+K shortcut
- Direct2D HWND render target and DirectWrite text rendering
- Event-driven 16 ms redraw timer with a short fade-out window

## Known limitations
- The overlay currently targets the primary display; independent overlays and placement per monitor are next.
- Animation is a basic event fade, not yet configurable.
- Settings are a scaffold; persistence, filter editor and history browser are not implemented.
- Mouse movement visualization and complete key-state/keyboard-layout handling remain to be implemented.
- The Direct2D HWND target uses global layered-window opacity, not per-pixel transparency.

## Next milestones
1. Verify the Actions build and runtime behavior on Windows 10 x64.
2. Add per-monitor overlay windows, DPI-aware placement and monitor hot-plug handling.
3. Implement configurable animation, filters, history browsing and persisted settings.
4. Complete native settings, tray integration and installer packaging.

This is an incremental development build, not yet a complete Keyviz replacement.
