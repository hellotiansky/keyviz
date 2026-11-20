# Keyviz Native (C++ rewrite)

This directory is the Windows-native rewrite track. The existing Tauri application
remains untouched while the native implementation is developed and validated.

## Toolchain
- Windows 10 x64
- Visual Studio 2022 (Desktop development with C++)
- Windows 10/11 SDK
- CMake 3.24+

## Local build

Run from a Visual Studio Developer PowerShell:

```powershell
cmake -S native -B build/native -A x64
cmake --build build/native --config Release
```

Output: `build/native/Release/KeyvizNative.exe`.

## Stage 1
- Win32 GUI entry point and transparent topmost click-through overlay
- Background keyboard/mouse Raw Input registration
- Basic key, mouse button and wheel event visualization
- Bounded in-memory history and filter seam
- Native Win32 settings window
- Ctrl+Shift+K settings shortcut

## Planned stages
1. Harden input normalization (left/right modifiers, repeat, device identity,
   layouts, injected input and Raw Input edge cases).
2. Replace temporary GDI with Direct2D/DirectWrite and add animation.
3. Add per-monitor windows, DPI awareness, hot-plug and placement settings.
4. Add configurable filters, history search/export and persistent settings.
5. Complete settings UI, tray integration, startup behavior and packaging.

Stage 1 is a buildable foundation, not yet a feature-complete replacement.
