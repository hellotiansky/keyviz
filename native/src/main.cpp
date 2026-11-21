#include <windows.h>
#include "keyviz/input_core.h"
#include "keyviz/input_event.h"
#include "keyviz/event_engine.h"
#include "keyviz/renderer.h"
#include "keyviz/settings_store.h"

#include <algorithm>
#include <memory>
#include <functional>
#include <deque>
#include <string>
#include <utility>
#include <vector>
#include <cwctype>

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
constexpr int kHistoryHotkeyId = 2;
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
            CreateWindowExW(0, L"BUTTON", L"Clear history",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 246, 195, 130, 30,
                hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kClearHistoryId)),
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
            }
            if (LOWORD(wParam) == kHideId) ShowWindow(hwnd, SW_HIDE);
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

class HistoryWindow {
public:
    bool create(HINSTANCE instance,
        std::function<const std::deque<InputEvent>&()> getEvents) {
        getEvents_ = std::move(getEvents);
        WNDCLASSEXW wc{sizeof(wc)};
        wc.lpfnWndProc = &HistoryWindow::windowProc;
        wc.hInstance = instance;
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
        wc.lpszClassName = L"KeyvizNativeHistory";
        if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
            return false;
        hwnd_ = CreateWindowExW(WS_EX_APPWINDOW, L"KeyvizNativeHistory",
            L"Keyviz Native - Input History",
            WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 620, 430,
            nullptr, nullptr, instance, this);
        return hwnd_ != nullptr;
    }

    void show() {
        if (!hwnd_) return;
        refresh();
        ShowWindow(hwnd_, SW_SHOW);
        SetForegroundWindow(hwnd_);
    }

    void refresh() const {
        if (!hwnd_ || !IsWindowVisible(hwnd_)) return;
        HWND list = GetDlgItem(hwnd_, kListId);
        HWND search = GetDlgItem(hwnd_, kSearchId);
        HWND typeFilter = GetDlgItem(hwnd_, kTypeFilterId);
        if (!list || !search || !typeFilter || !getEvents_) return;
        SendMessageW(list, LB_RESETCONTENT, 0, 0);
        int totalCount = 0;
        int visibleCount = 0;
        const int selectedType = static_cast<int>(SendMessageW(typeFilter, CB_GETCURSEL, 0, 0));
        wchar_t searchBuffer[256]{};
        GetWindowTextW(search, searchBuffer, static_cast<int>(std::size(searchBuffer)));
        const std::wstring query(searchBuffer);
        const auto& events = getEvents_();
        const DWORD now = GetTickCount();
        for (const auto& event : events) {
            ++totalCount;
            const wchar_t* type = L"KEY";
            switch (event.type) {
            case InputEvent::Type::KeyDown: type = L"KEY DOWN"; break;
            case InputEvent::Type::KeyUp: type = L"KEY UP"; break;
            case InputEvent::Type::MouseDown: type = L"MOUSE DOWN"; break;
            case InputEvent::Type::MouseUp: type = L"MOUSE UP"; break;
            case InputEvent::Type::Wheel: type = L"WHEEL"; break;
            case InputEvent::Type::MouseMove: type = L"MOUSE MOVE"; break;
            }
            std::wstring searchable = event.label + L" " + event.chordLabel;
            if (!matchesType(event, selectedType) || !containsInsensitive(searchable, query))
                continue;
            ++visibleCount;
            const DWORD age = now - event.timestamp;
            std::wstring line = L"[" + std::to_wstring(age) + L" ms ago]  ";
            line += type;
            line += L"    ";
            line += event.chordLabel.empty() ? event.label : event.chordLabel;
            if (event.repeatCount > 1)
                line += L"  ×" + std::to_wstring(event.repeatCount);
            SendMessageW(list, LB_ADDSTRING, 0,
                reinterpret_cast<LPARAM>(line.c_str()));
        }
        if (visibleCount == 0) {
            const wchar_t* emptyMessage = totalCount == 0
                ? L"No input events recorded yet."
                : L"No events match the current search and filter.";
            SendMessageW(list, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(emptyMessage));
        }
        if (HWND status = GetDlgItem(hwnd_, kStatusId)) {
            std::wstring summary = std::to_wstring(visibleCount) + L" of " +
                std::to_wstring(totalCount) + L" events shown";
            SetWindowTextW(status, summary.c_str());
        }
    }

private:
    static constexpr int kSearchId = 301;
    static constexpr int kTypeFilterId = 302;
    static constexpr int kListId = 303;
    static constexpr int kStatusId = 304;

    static bool containsInsensitive(const std::wstring& value, const std::wstring& query) {
        if (query.empty()) return true;
        std::wstring lhs(value), rhs(query);
        std::transform(lhs.begin(), lhs.end(), lhs.begin(), towupper);
        std::transform(rhs.begin(), rhs.end(), rhs.begin(), towupper);
        return lhs.find(rhs) != std::wstring::npos;
    }

    static bool matchesType(const InputEvent& event, int filter) {
        switch (filter) {
        case 1: return event.type == InputEvent::Type::KeyDown ||
                       event.type == InputEvent::Type::KeyUp;
        case 2: return event.type == InputEvent::Type::MouseDown ||
                       event.type == InputEvent::Type::MouseUp;
        case 3: return event.type == InputEvent::Type::Wheel;
        case 4: return event.type == InputEvent::Type::MouseMove;
        default: return true;
        }
    }
    static LRESULT CALLBACK windowProc(HWND hwnd, UINT message,
        WPARAM wParam, LPARAM lParam) {
        auto* self = reinterpret_cast<HistoryWindow*>(
            GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (message == WM_NCCREATE) {
            const auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
            self = static_cast<HistoryWindow*>(create->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA,
                reinterpret_cast<LONG_PTR>(self));
        }
        if (!self) return DefWindowProcW(hwnd, message, wParam, lParam);
        switch (message) {
        case WM_CREATE: {
            const HINSTANCE instance = GetModuleHandleW(nullptr);
            CreateWindowExW(0, L"STATIC", L"Search:", WS_CHILD | WS_VISIBLE,
                10, 10, 55, 24, hwnd, nullptr, instance, nullptr);
            CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", nullptr,
                WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                68, 8, 250, 26, hwnd,
                reinterpret_cast<HMENU>(static_cast<INT_PTR>(kSearchId)),
                instance, nullptr);
            HWND combo = CreateWindowExW(0, L"COMBOBOX", nullptr,
                WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
                330, 8, 180, 200, hwnd,
                reinterpret_cast<HMENU>(static_cast<INT_PTR>(kTypeFilterId)),
                instance, nullptr);
            const wchar_t* options[] = {L"All events", L"Keyboard", L"Mouse buttons",
                L"Mouse wheel", L"Mouse movement"};
            for (const auto* option : options)
                SendMessageW(combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(option));
            SendMessageW(combo, CB_SETCURSEL, 0, 0);
            CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", nullptr,
                WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL |
                    LBS_NOINTEGRALHEIGHT | LBS_HASSTRINGS,
                10, 42, 580, 300, hwnd,
                reinterpret_cast<HMENU>(static_cast<INT_PTR>(kListId)),
                instance, nullptr);
            CreateWindowExW(0, L"STATIC", L"0 of 0 events shown",
                WS_CHILD | WS_VISIBLE, 10, 376, 580, 20, hwnd,
                reinterpret_cast<HMENU>(static_cast<INT_PTR>(kStatusId)),
                instance, nullptr);
            return 0;
        }
        case WM_COMMAND:
            if ((LOWORD(wParam) == kSearchId && HIWORD(wParam) == EN_CHANGE) ||
                (LOWORD(wParam) == kTypeFilterId && HIWORD(wParam) == CBN_SELCHANGE)) {
                self->refresh();
                return 0;
            }
            break;
        case WM_SIZE: {
            const int width = std::max(0, static_cast<int>(LOWORD(lParam)) - 20);
            const int clientHeight = static_cast<int>(HIWORD(lParam));
            const int height = std::max(0, clientHeight - 90);
            if (HWND search = GetDlgItem(hwnd, kSearchId))
                MoveWindow(search, 68, 8, std::max(120, width - 330), 26, TRUE);
            if (HWND filter = GetDlgItem(hwnd, kTypeFilterId))
                MoveWindow(filter, std::max(330, width - 180), 8, 180, 200, TRUE);
            if (HWND list = GetDlgItem(hwnd, kListId))
                MoveWindow(list, 10, 42, width, height, TRUE);
            if (HWND status = GetDlgItem(hwnd, kStatusId))
                MoveWindow(status, 10, std::max(42, clientHeight - 26), width, 20, TRUE);
            return 0;
        }
        case WM_CLOSE:
            ShowWindow(hwnd, SW_HIDE);
            return 0;
        }
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }

    HWND hwnd_{};
    std::function<const std::deque<InputEvent>&()> getEvents_;
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
        if (!historyWindow_.create(instance_, [this]() -> const std::deque<InputEvent>& {
                return engine_.history().events();
            })) return false;
        if (!settings_.create(instance_, initialFilters, [this](const FilterSettings& filters) {
                if (settingsStore_.save(filters)) {
                    engine_.setFilterSettings(filters);
                } else {
                    MessageBoxW(inputWindow_, L"Could not save settings.",
                        kAppName, MB_ICONWARNING | MB_OK);
                }
            }, [this]() {
                engine_.clearHistory();
                historyWindow_.refresh();
                KillTimer(inputWindow_, kRedrawTimerId);
                invalidateOverlays();
            })) return false;
        if (!RegisterHotKey(inputWindow_, kHistoryHotkeyId, MOD_CONTROL | MOD_SHIFT, 'H')) {
            MessageBoxW(nullptr, L"Ctrl+Shift+H could not be registered.",
                kAppName, MB_ICONWARNING | MB_OK);
        }
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
        historyWindow_.refresh();
        SetTimer(inputWindow_, kRedrawTimerId, 16, nullptr);
        invalidateOverlays();
    }

    void shutdown() noexcept {
        if (inputWindow_) {
            KillTimer(inputWindow_, kRedrawTimerId);
            UnregisterHotKey(inputWindow_, kSettingsHotkeyId);
            UnregisterHotKey(inputWindow_, kHistoryHotkeyId);
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
            if (isInputWindow && wParam == kHistoryHotkeyId)
                self->historyWindow_.show();
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
    HistoryWindow historyWindow_;
    std::vector<std::unique_ptr<OverlayWindow>> overlays_;
};
} // namespace keyviz

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int) {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    keyviz::App app(instance);
    if (!app.initialize()) return 1;
    return app.run();
}
