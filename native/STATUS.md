# Native rewrite status

- Track: Windows-native C++20 rewrite
- Build: CMake + MSVC x64
- CI: `.github/workflows/native-windows.yml`
- Completed: Win32 overlay and per-monitor DPI awareness request
- Completed: one independent overlay HWND and Direct2D renderer per enumerated display
- Completed: single hidden Raw Input owner window; shared event history is rendered by all overlays
- Completed: display topology change rebuilds the overlay set
- Completed: extracted Raw Input capture and normalized keyboard/mouse button/wheel events
- Completed: coalesced mouse movement samples with virtual-desktop cursor coordinates
- Completed: InputEvent, FilterEngine, HistoryStore and EventEngine modules
- In progress: robust keyboard layout/modifier handling and configurable animation
- Next: persistent native settings, filter editor, history browser and tray integration

The original Tauri application remains intact on the base branch.
