# Native rewrite status

- Track: Windows-native C++20 rewrite
- Build: CMake + MSVC x64
- CI: `.github/workflows/native-windows.yml` (x64 Release build, CTest, portable executable and Inno Setup installer artifacts)
- Completed: Win32 overlay and per-monitor DPI awareness request
- Completed: one independent overlay HWND and Direct2D renderer per enumerated display
- Completed: single hidden Raw Input owner window; shared event history is rendered by all overlays
- Completed: display topology change rebuilds the overlay set
- Completed: normalized keyboard, mouse button and wheel capture; sampled mouse movement
- Completed: InputEvent, FilterEngine, HistoryStore and EventEngine modules
- Completed: native settings window, persisted event filters and configurable 40/80/160/320-event history capacity
- Completed: searchable/filterable history browser, event counts, empty states and filtered UTF-8 CSV export
- Completed: CTest coverage for history capacity, resizing, clearing, event coalescing and persistence round-trips
- Completed: Raw Input core tests for left/right Ctrl normalization, modifier chord state and AltGr combinations
- Completed: versioned, bounded event-history persistence with atomic file replacement, 30-second checkpoints and shutdown save
- Completed: notification-area tray icon with settings/history shortcuts and clean exit
- Completed: configurable overlay fade duration (0.4/0.8/1.1/1.6 seconds), persisted in settings
- Completed: key translation now uses the app's tracked Raw Input modifier state (including left/right modifiers and AltGr state)
- Completed: E0 extended scan-code prefix is passed to keyboard-layout translation
- Completed: printable dead-key accents are surfaced with a `(dead)` label without consuming layout composition state
- Documented: IME integration options and privacy boundary in `native/docs/IME_AND_TEXT_INPUT.md`
- In progress: locale-specific dead-key sequences, remaining keyboard-layout edge cases and richer animation styles
- IME limitation: global composition and committed-text capture remain intentionally unsupported; do not read arbitrary foreground document text or inject hooks into other processes
- Persistence limitation: forced termination can lose events since the last 30-second checkpoint
- Completed: Inno Setup per-user x64 installer, Start Menu shortcut, optional desktop shortcut and registered uninstaller
- Completed: embedded Windows file/product version metadata for the native executable
- Completed: branded multi-size Windows icon generated reproducibly from PowerShell and embedded in the executable and notification-area icon
- Next: physical-device validation, including dead-key and IME coexistence
- Runtime validation still needed on physical multi-monitor Windows systems, including mixed-DPI and hot-plug configurations

The original Tauri application remains intact on the base branch.
