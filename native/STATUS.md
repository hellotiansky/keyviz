# Native rewrite status

- Track: Windows-native C++20 rewrite
- Build: CMake + MSVC x64
- CI: `.github/workflows/native-windows.yml` (x64 Release build, CTest, executable artifact)
- Completed: Win32 overlay and per-monitor DPI awareness request
- Completed: one independent overlay HWND and Direct2D renderer per enumerated display
- Completed: single hidden Raw Input owner window; shared event history is rendered by all overlays
- Completed: display topology change rebuilds the overlay set
- Completed: normalized keyboard, mouse button and wheel capture; sampled mouse movement
- Completed: InputEvent, FilterEngine, HistoryStore and EventEngine modules
- Completed: native settings window, persisted event filters and configurable 40/80/160/320-event history capacity
- Completed: searchable/filterable history browser, event counts, empty states and filtered UTF-8 CSV export
- Completed: CTest coverage for history capacity, resizing, clearing and event coalescing
- In progress: robust keyboard layout/modifier handling and configurable animation
- Next: durable event-history persistence, tray integration and installer packaging
- Runtime validation still needed on physical multi-monitor Windows systems, including mixed-DPI and hot-plug configurations

The original Tauri application remains intact on the base branch.
