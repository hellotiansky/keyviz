# IME and text-input integration assessment

## Current behavior

Keyviz Native registers keyboard and mouse Raw Input on one hidden top-level window. Raw Input reports physical key transitions and scan codes; it does not forward another process's IME composition string, candidate list, or committed text to this window. The current renderer therefore visualizes physical input, not arbitrary text entered into other applications.

For each translated key, the implementation resolves the current foreground window's owning thread and obtains that thread's active `HKL` with `GetKeyboardLayout`; it does not cache the layout. Consequently, a foreground application's input-language switch is reflected on the next key translation without maintaining a separate `WM_INPUTLANGCHANGE` cache-invalidation path. If there is no foreground window, the call uses the capture thread's current layout (`GetKeyboardLayout(0)`). This selects a layout for key-label translation only; it does not make the hidden Raw Input window an IME client for the foreground application.

Keyboard-layout translation uses `ToUnicodeEx` with `TU_NO_STATE_CHANGE` (`0x4`) so looking up a key label does not consume or mutate the foreground layout's pending dead-key composition state. A negative `ToUnicodeEx` result identifies a dead key; the native label now exposes its printable accent with a `(dead)` suffix. Extended E0 and E1 scan-code prefixes are preserved for layout translation (including the special E1-prefixed key sequences). Translated Unicode labels are filtered for control code points without relying on the C runtime's locale-sensitive `iswprint`, so layouts producing CJK characters remain supported even when the process locale is still the default `C` locale.

## Options considered

| Approach | What it can provide | Constraints |
|---|---|---|
| Raw Input + `ToUnicodeEx` | Physical keys and layout-aware printable key labels | Does not provide another application's IME composition or committed text; dead-key output is layout-dependent |
| IMM32 messages (`WM_IME_*`) | Composition and result strings delivered to an IME client window | Messages are delivered to the target application's window, not a separate global observer; subclassing/injecting into other processes is not an acceptable default architecture |
| Text Services Framework (TSF) | Rich text-service composition and document-manager integration | A TSF text service/client integration is substantially more invasive than a passive global input listener and still does not grant arbitrary cross-process document access |
| UI Automation text patterns | In some controls, access to text or selection exposed by the target provider | Provider-dependent, potentially broad access to sensitive document contents, and not equivalent to a reliable stream of committed keystrokes |

## Decision

- Keep physical-key visualization as the default and supported capture model.
- Do not read arbitrary focused-control text, enumerate candidate windows, inject hooks into other processes, or persist IME text as a side effect of enabling keyboard visualization.
- If text visualization is ever added, make it a separate, explicit opt-in feature with a clear active indicator, a narrow capture scope, no persistence/export by default, and a dedicated privacy review.
- Prefer an in-process TSF/IMM32 integration only for a future Keyviz-owned text-entry surface. Do not describe it as global IME capture.
- Continue validating dead-key behavior across layouts on real Windows systems; CI cannot validate locale-specific IME composition behavior.

## Follow-up validation

1. Open two ordinary foreground applications with different input languages installed. Keep capture running, switch the active language in the first application, type a printable key, then switch to the second application and repeat. The next key label should follow the active foreground thread's layout rather than a layout cached when capture started.
2. With a layout that has a dead key, press the accent key and then its base letter. Confirm Keyviz can label the accent as `(dead)` while the target application still receives the expected composed character; repeat with capture stopped as a control.
3. Test AltGr, E0/E1 extended keys (including Pause/Break behavior) on physical keyboards and record keyboard model, Windows version and active layout for any discrepancy.
4. If there is no foreground application (for example, immediately after closing the active window), check that translation falls back to the capture thread's layout and does not crash.
5. If a future opt-in text feature is proposed, define its exact scope and retention behavior before implementing any cross-process integration.