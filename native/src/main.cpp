#include <windows.h>
#include "keyviz/input_core.h"
#include "keyviz/input_event.h"
#include "keyviz/event_engine.h"
#include "keyviz/renderer.h"
#include "keyviz/settings_store.h"

#include <algorithm>
#include <memory>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace keyviz {
constexpr wchar_t kAppName[] = L"Keyviz Native";
constexpr wchar_t kOverlayClass[] = L"KeyvizNativeOverlay";
constexpr wchar_t kInputClass[] = L"KeyvizNativeInput";
constexpr wchar_t kSettingsClass[] = L"KeyvizNativeSettings";
constexpr int kOverlayWidth = 580;
constexpr int kOverlayHeight = 156;
constexpr int kOverlayBottomMargin = 36;
constexpr int kSettingsButtonId = 100;
constexpr int kSettingsHotkeyId = 1;
constexpr UINT_PTR kRedrawTimerId = 2;

class SettingsWindow {
public:
    bool create(HINSTANCE instance, const FilterSettings& initial,
        std::function<void(const FilterSettings&)> onApply,
        std::function<void()> onClearHistory) {
        settings_ = initial;
        onApply_ = std::move(onApply);
        onClearHistory_ = std::move(onClearHistory);
        WNDCLASSEXW wc{sizeof(wc)};
        wc.lpfnWndProc = &SettingsWindow::windowProc;
        wc.hInstance = instance;
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
        wc.lpszClassName = kSettingsClass;
        if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
            return false;
        hwnd_ = CreateWindowExW(WS_EX_APPWINDOW, kSettingsClass,
            L"Keyviz Native - Settings",
            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
            CW_USEDEFAULT, CW_USEDEFAULT, 460, 300,
            nullptr, nullptr, instance, this);
        return hwnd_ != nullptr;
    }
    void show() const {
        if (!hwnd_) return;
        ShowWindow(hwnd_, SW_SHOW);
        SetForegroundWindow(hwnd_);
    }
private:
    enum : int {
        kKeyboardFilterId = 201, kMouseButtonsFilterId = 202,
        kWheelFilterId = 203, kMouseMoveFilterId = 204,
        kApplyId = 205, kHideId = 206, kClearHistoryId = 207
    };

    static void createCheckBox(HWND hwnd, HINSTANCE instance,
        const wchar_t* label, int id, int y, bool checked) {
        HWND control = CreateWindowExW(0, L"BUTTON", label,
            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
            22, y, 380, 25, hwnd,
            reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
            instance, nullptr);
        SendMessageW(control, BM_SETCHECK,
            checked ? BST_CHECKED : BST_UNCHECKED, 0);
    }

    static LRESULT CALLBACK windowProc(HWND hwnd, UINT message,
        WPARAM wParam, LPARAM lParam) {
        auto* self = reinterpret_cast<SettingsWindow*>(
            GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (message == WM_NCCREATE) {
            const auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
            self = static_cast<SettingsWindow*>(create->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA,
                reinterpret_cast<LONG_PTR>(self));
        }
        if (!self) return DefWindowProcW(hwnd, message, wParam, lParam);
        switch (message) {
        case WM_CREATE: {
            const HINSTANCE instance = GetModuleHandleW(nullptr);
            CreateWindowExW(0, L"STATIC", L"Choose which input events appear in the overlay:",
                WS_CHILD | WS_VISIBLE, 20, 18, 410, 22,
                hwnd, nullptr, instance, nullptr);
            createCheckBox(hwnd, instance, L"Keyboard", kKeyboardFilterId, 52,
                self->settings_.keyboard);
            createCheckBox(hwnd, instance, L"Mouse buttons", kMouseButtonsFilterId, 82,
                self->settings_.mouseButtons);
            createCheckBox(hwnd, instance, L"Mouse wheel", kWheelFilterId, 112,
                self->settings_.mouseWheel);
            createCheckBox(hwnd, instance, L"Mouse movement", kMouseMoveFilterId, 142,
                self->settings_.mouseMove);
            CreateWindowExW(0, L"BUTTON", L"Apply",
                WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 22, 195, 100, 30,
                hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kApplyId)),
                instance, nullptr);
            CreateWindowExW(0, L"BUTTON", L"Hide",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 134, 195, 100, 30,
                hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kHideId)),
                instance, nullptr);
            return 0;
        }
        case WM_COMMAND:
            if (LOWORD(wParam) == kApplyId) {
                FilterSettings next;
                next.keyboard = IsDlgButtonChecked(hwnd, kKeyboardFilterId) == BST_CHECKED;
                next.mouseButtons = IsDlgButtonChecked(hwnd, kMouseButtonsFilterId) == BST_CHECKED;
                next.mouseWheel = IsDlgButtonChecked(hwnd, kWheelFilterId) == BST_CHECKED;
                next.mouseMove = IsDlgButtonChecked(hwnd, kMouseMoveFilterId) == BST_CHECKED;
                if (self->onApply_) self->onApply_(next);
                return 0;
            }
            if (LOWORD(wParam) == kClearHistoryId) {
                if (self->onClearHistory_) self->onClearHistory_();
                return 0;
            }\n            if (LOWORD(wParam) == kHideId) ShowWindow(hwnd, SW_HIDE);
            return 0;
        case WM_CLOSE:
            ShowWindow(hwnd, SW_HIDE);
            return 0;
        }
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }

    HWND hwnd_{};
    FilterSettings settings_{};
    std::function<void(const FilterSettings&)> onApply_;
    std::function<void()> onClearHistory_;
};

struct OverlayWindow {
    HWND hwnd{};
    Renderer renderer;
};

class App {
public:
    explicit App(HINSTANCE instance) : instance_(instance) {}

    bool initialize() {
        if (!registerWindowClasses()) return false;
        const auto initialFilters = settingsStore_.load();
        engine_.setFilterSettings(initialFilters);

        // A single hidden top-level window owns Raw Input and the global hotkey.
        // Overlay windows are presentation-only and never register input devices.
        inputWindow_ = CreateWindowExW(
            WS_EX_TOOLWINDOW, kInputClass, kAppName, WS_POPUP,
            0, 0, 0, 0, nullptr, nullptr, instance_, this);
        if (!inputWindow_) return false;

        if (!input_.start(inputWindow_)) {
            MessageBoxW(nullptr,
                L"Could not register keyboard and mouse Raw Input devices.",
                kAppName, MB_ICONERROR | MB_OK);
            return false;
        }

        if (!rebuildOverlays()) {
            MessageBoxW(nullptr, L"No display could host a Keyviz overlay.",
                kAppName, MB_ICONERROR | MB_OK);
            return false;
        }
        if (!settings_.create(instance_, initialFilters, [this](const FilterSettings& filters) {
                if (settingsStore_.save(filters)) {
                    engine_.setFilterSettings(filters);
                } else {
                    MessageBoxW(inputWindow_, L"Could not save settings.",
                        kAppName, MB_ICONWARNING | MB_OK);
                }
            }, [this]() {
                engine_.clearHistory();
                KillTimer(inputWindow_, kRedrawTimerId);
                invalidateOverlays();
            })) return false;
        if (!RegisterHotKey(inputWindow_, kSettingsHotkeyId,
                MOD_CONTROL | MOD_SHIFT, 'K')) {
            MessageBoxW(nullptr, L"Ctrl+Shift+K could not be registered.",
                kAppName, MB_ICONWARNING | MB_OK);
        }
        return true;
    }

    int run() {
        MSG message{};
        while (GetMessageW(&message, nullptr, 0, 0) > 0) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
        shutdown();
        return static_cast<int>(message.wParam);
    }

private:
    bool registerWindowClasses() {
        WNDCLASSEXW overlayClass{sizeof(overlayClass)};
        overlayClass.lpfnWndProc = &App::windowProc;
        overlayClass.hInstance = instance_;
        overlayClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        overlayClass.lpszClassName = kOverlayClass;
        if (!RegisterClassExW(&overlayClass) &&
            GetLastError() != ERROR_CLASS_ALREADY_EXISTS) return false;

        WNDCLASSEXW inputClass{sizeof(inputClass)};
        inputClass.lpfnWndProc = &App::windowProc;
        inputClass.hInstance = instance_;
        inputClass.lpszClassName = kInputClass;
        if (!RegisterClassExW(&inputClass) &&
            GetLastError() != ERROR_CLASS_ALREADY_EXISTS) return false;
        return true;
    }

    static BOOL CALLBACK monitorCallback(
        HMONITOR monitor, HDC, LPRECT, LPARAM context) {
        auto* self = reinterpret_cast<App*>(context);
        return self->createOverlay(monitor) ? TRUE : FALSE;
    }

    bool createOverlay(HMONITOR monitor) {
        MONITORINFO info{sizeof(info)};
        if (!GetMonitorInfoW(monitor, &info)) return false;

        const RECT work = info.rcWork;
        const int workWidth = work.right - work.left;
        const int workHeight = work.bottom - work.top;
        const int width = std::max(1, std::min(kOverlayWidth, workWidth - 40));
        const int height = std::max(1, std::min(kOverlayHeight, workHeight - 20));
        const int x = work.left + (workWidth - width) / 2;
        const int y = work.bottom - height - kOverlayBottomMargin;

        HWND hwnd = CreateWindowExW(
            WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW,
            kOverlayClass, kAppName, WS_POPUP, x, y, width, height,
            nullptr, nullptr, instance_, this);
        if (!hwnd) return false;

        auto overlay = std::make_unique<OverlayWindow>();
        overlay->hwnd = hwnd;
        if (!overlay->renderer.initialize(hwnd)) {
            DestroyWindow(hwnd);
            return false;
        }
        SetLayeredWindowAttributes(hwnd, 0, 242, LWA_ALPHA);
        overlays_.push_back(std::move(overlay));
        ShowWindow(hwnd, SW_SHOWNOACTIVATE);
        UpdateWindow(hwnd);
        return true;
    }

    bool rebuildOverlays() {
        for (auto& overlay : overlays_) {
            overlay->renderer.shutdown();
            if (IsWindow(overlay->hwnd)) DestroyWindow(overlay->hwnd);
        }
        overlays_.clear();

        if (!EnumDisplayMonitors(nullptr, nullptr,
                &App::monitorCallback, reinterpret_cast<LPARAM>(this))) {
            // EnumDisplayMonitors returns FALSE if enumeration was interrupted.
            return false;
        }
        return !overlays_.empty();
    }

    OverlayWindow* findOverlay(HWND hwnd) noexcept {
        const auto it = std::find_if(overlays_.begin(), overlays_.end(),
            [hwnd](const auto& overlay) { return overlay->hwnd == hwnd; });
        return it == overlays_.end() ? nullptr : it->get();
    }

    void invalidateOverlays() const noexcept {
        for (const auto& overlay : overlays_)
            InvalidateRect(overlay->hwnd, nullptr, FALSE);
    }

    void handleRawInput(HRAWINPUT handle) {
        RAWINPUT raw{};
        if (!InputCore::read(handle, raw)) return;
        auto events = InputCore::decode(raw);
        if (events.empty()) return;
        for (auto& event : events) engine_.process(std::move(event));
        SetTimer(inputWindow_, kRedrawTimerId, 16, nullptr);
        invalidateOverlays();
    }

    void shutdown() noexcept {
        if (inputWindow_) {
            KillTimer(inputWindow_, kRedrawTimerId);
            UnregisterHotKey(inputWindow_, kSettingsHotkeyId);
        }
        input_.stop();
        for (auto& overlay : overlays_) {
            overlay->renderer.shutdown();
            if (IsWindow(overlay->hwnd)) DestroyWindow(overlay->hwnd);
        }
        overlays_.clear();
        if (inputWindow_ && IsWindow(inputWindow_)) DestroyWindow(inputWindow_);
        inputWindow_ = nullptr;
    }

    static LRESULT CALLBACK windowProc(HWND hwnd, UINT message,
        WPARAM wParam, LPARAM lParam) {
        App* self = reinterpret_cast<App*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (message == WM_NCCREATE) {
            const auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
            self = static_cast<App*>(create->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }
        if (!self) return DefWindowProcW(hwnd, message, wParam, lParam);

        const bool isInputWindow = hwnd == self->inputWindow_;
        switch (message) {
        case WM_INPUT:
            if (isInputWindow)
                self->handleRawInput(reinterpret_cast<HRAWINPUT>(lParam));
            // Required cleanup for foreground WM_INPUT messages.
            return DefWindowProcW(hwnd, message, wParam, lParam);
        case WM_HOTKEY:
            if (isInputWindow && wParam == kSettingsHotkeyId)
                self->settings_.show();
            return 0;
        case WM_PAINT: {
            PAINTSTRUCT paint{};
            BeginPaint(hwnd, &paint);
            if (auto* overlay = self->findOverlay(hwnd))
                overlay->renderer.render(self->engine_.history().events());
            EndPaint(hwnd, &paint);
            return 0;
        }
        case WM_SIZE:
            if (auto* overlay = self->findOverlay(hwnd)) {
                overlay->renderer.resize(LOWORD(lParam), HIWORD(lParam));
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            return 0;
        case WM_TIMER:
            if (isInputWindow && wParam == kRedrawTimerId) {
                const auto& events = self->engine_.history().events();
                if (events.empty() ||
                    GetTickCount() - events.front().timestamp > 1150) {
                    KillTimer(hwnd, kRedrawTimerId);
                }
                self->invalidateOverlays();
            }
            return 0;
        case WM_DISPLAYCHANGE:
            if (isInputWindow) self->rebuildOverlays();
            return 0;
        case WM_DESTROY:
            if (isInputWindow) PostQuitMessage(0);
            return 0;
        }
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }

    HINSTANCE instance_{};
    HWND inputWindow_{};
    InputCore input_;
    EventEngine engine_;
    SettingsStore settingsStore_;
    SettingsWindow settings_;
    std::vector<std::unique_ptr<OverlayWindow>> overlays_;
};
} // namespace keyviz

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int) {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    keyviz::App app(instance);
    if (!app.initialize()) return 1;
    return app.run();
}
