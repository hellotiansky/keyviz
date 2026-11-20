# Native rewrite status

- Track: Windows-native C++20 rewrite
- Build: CMake + MSVC x64
- CI: `.github/workflows/native-windows.yml`
- Completed: Stage 1 Win32 overlay, Raw Input, bounded event history and native settings scaffold
- Completed: Direct2D/DirectWrite renderer integration and event fade redraw loop
- In progress: modular input/event architecture and Windows runtime validation
- Next: per-monitor overlay windows, per-monitor DPI placement, monitor hot-plug, configurable animation and persistent settings

The original Tauri application remains intact on the base branch.
