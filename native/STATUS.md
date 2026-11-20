# Native rewrite status

- Track: Windows-native C++20 rewrite
- Build: CMake + MSVC x64
- CI: `.github/workflows/native-windows.yml`
- Completed: Win32 overlay, Raw Input foundation, native settings scaffold
- Completed: Direct2D/DirectWrite renderer and event fade redraw loop
- Completed: extracted InputEvent, FilterEngine, HistoryStore and EventEngine modules
- In progress: Windows Actions verification and input normalization hardening
- Next: per-monitor overlay windows, DPI-aware placement, monitor hot-plug, configurable animation and persistent settings

The original Tauri application remains intact on the base branch.
