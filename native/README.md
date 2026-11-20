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
- C++20 Win32 GUI application
- Per-monitor DPI awareness V2 request at process startup
- Transparent, topmost, click-through overlay
- Background keyboard and mouse Raw Input
- Keyboard, mouse button and wheel event normalization (initial subset)
- Bounded in-memory history and a filter seam
- Native Win32 settings window; Ctrl+Shift+K shortcut
- Direct2D HWND render target and DirectWrite text rendering
- Event-driven 16 ms redraw timer, stopped after the current event's fade window

## Known limitations
- The overlay currently targets the primary display; independent overlays and placement per monitor are next.
- Animation is currently a simple event fade, not yet configurable.
- Settings are a scaffold; settings persistence, filter editor and history browser are not implemented.
- Mouse movement visualization and complete key-state/keyboard-layout handling remain to be implemented.
- Direct2D uses an HWND render target; the current layered-window alpha is global opacity, not per-pixel transparency.

## Next milestones
1. Verify the Actions build and runtime behavior on Windows 10 x64.
2. Split input capture, event processing and history into dedicated translation units.
3. Add per-monitor overlay windows, DPI-aware placement and monitor hot-plug handling.
4. Implement configurable animations, filters, history browsing and persisted settings.
5. Complete native settings, tray integration and installer packaging.

This is an incremental development build, not yet a complete Keyviz replacement.
