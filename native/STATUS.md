# Native rewrite status

- Track: Windows-native C++20 rewrite
- Build: CMake + MSVC x64
- CI: `.github/workflows/native-windows.yml`
- Completed: Win32 overlay and per-monitor DPI awareness request
- Completed: one independent overlay HWND and Direct2D renderer per enumerated display
- Completed: single hidden Raw Input owner window; shared event history is rendered by all overlays
- Completed: display topology change rebuilds the overlay set
- Completed: extracted Raw Input capture and normalized keyboard/mouse button/wheel events
- Completed: InputEvent, FilterEngine, HistoryStore and EventEngine modules
- In progress: runtime verification on mixed-DPI and hot-plug multi-monitor Windows systems
- Next: configurable animation, persistent settings, filter editor and history browser

The original Tauri application remains intact on the base branch.
