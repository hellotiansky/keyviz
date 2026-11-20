#include <windows.h>
#include <hidusage.h>
#include "keyviz/input_event.h"
#include "keyviz/event_engine.h"
#include "keyviz/renderer.h"

#include <deque>
#include <iterator>
#include <string>
#include <utility>

namespace keyviz {
constexpr wchar_t kAppName[] = L"Keyviz Native";
constexpr wchar_t kOverlayClass[] = L"KeyvizNativeOverlay";
constexpr wchar_t kSettingsClass[] = L"KeyvizNativeSettings";
constexpr UINT kMaxHistory = 80;
constexpr int kOverlayWidth = 580;
constexpr int kOverlayHeight = 156;
constexpr int kSettingsButtonId = 100;
constexpr int kSettingsHotkeyId = 1;

class InputCore {
public:
    bool start(HWND target) {
        RAWINPUTDEVICE devices[2]{};
        devices[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
        devices[0].usUsage = HID_USAGE_GENERIC_KEYBOARD;
        devices[0].dwFlags = RIDEV_INPUTSINK;
        devices[0].hwndTarget = target;
        devices[1].usUsagePage = HID_USAGE_PAGE_GENERIC;
        devices[1].usUsage = HID_USAGE_GENERIC_MOUSE;
        devices[1].dwFlags = RIDEV_INPUTSINK;
        devices[1].hwndTarget = target;
        registered_ = RegisterRawInputDevices(
            devices, static_cast<UINT>(std::size(devices)),
            sizeof(RAWINPUTDEVICE)) == TRUE;
        return registered_;
    }

    void stop() noexcept {
        if (!registered_) return;
        RAWINPUTDEVICE devices[2]{};
        devices[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
        devices[0].usUsage = HID_USAGE_GENERIC_KEYBOARD;
        devices[0].dwFlags = RIDEV_REMOVE;
        devices[1].usUsagePage = HID_USAGE_PAGE_GENERIC;
        devices[1].usUsage = HID_USAGE_GENERIC_MOUSE;
        devices[1].dwFlags = RIDEV_REMOVE;
        RegisterRawInputDevices(devices,
            static_cast<UINT>(std::size(devices)), sizeof(RAWINPUTDEVICE));
        registered_ = false;
    }

    static bool read(HRAWINPUT handle, RAWINPUT& raw) noexcept {
        UINT size = sizeof(raw);
        return GetRawInputData(handle, RID_INPUT, &raw, &size,
            sizeof(RAWINPUTHEADER)) != static_cast<UINT>(-1);
    }

    static std::wstring keyName(const RAWKEYBOARD& key) {
        if (key.VKey == 255) return {};
        const LONG keyData = static_cast<LONG>(
            (key.MakeCode << 16) | ((key.Flags & RI_KEY_E0) ? (1L << 24) : 0));
        wchar_t name[128]{};
        if (GetKeyNameTextW(keyData, name,
                static_cast<int>(std::size(name))) > 0) return name;
        return L"Key " + std::to_wstring(key.VKey);
    }
private:
    bool registered_{false};
};

class SettingsWindow {
public:
    bool create(HINSTANCE instance) {
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
            CW_USEDEFAULT, CW_USEDEFAULT, 460, 260,
            nullptr, nullptr, instance, this);
        return hwnd_ != nullptr;
    }
    void show() const {
        if (!hwnd_) return;
        ShowWindow(hwnd_, SW_SHOW);
        SetForegroundWindow(hwnd_);
    }
private:
    static LRESULT CALLBACK windowProc(HWND hwnd, UINT message,
        WPARAM wParam, LPARAM lParam) {
        if (message == WM_CREATE) {
            const HINSTANCE instance = GetModuleHandleW(nullptr);
            CreateWindowExW(0, L"STATIC",
                L"Keyviz Native settings (initial scaffold)",
                WS_CHILD | WS_VISIBLE, 20, 20, 400, 24,
                hwnd, nullptr, instance, nullptr);
            CreateWindowExW(0, L"STATIC",
                L"Input: Raw Input | Renderer: Win32 GDI (temporary)",
                WS_CHILD | WS_VISIBLE, 20, 58, 400, 24,
                hwnd, nullptr, instance, nullptr);
            CreateWindowExW(0, L"STATIC",
                L"Shortcut: Ctrl + Shift + K",
                WS_CHILD | WS_VISIBLE, 20, 96, 400, 24,
                hwnd, nullptr, instance, nullptr);
            CreateWindowExW(0, L"BUTTON", L"Hide",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 20, 150, 100, 30,
                hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kSettingsButtonId)),
                instance, nullptr);
            return 0;
        }
        switch (message) {
        case WM_COMMAND:
            if (LOWORD(wParam) == kSettingsButtonId) ShowWindow(hwnd, SW_HIDE);
            return 0;
        case WM_CLOSE:
            ShowWindow(hwnd, SW_HIDE);
            return 0;
        }
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }
    HWND hwnd_{};
};

class App {
public:
    explicit App(HINSTANCE instance) : instance_(instance) {}

    bool initialize() {
        WNDCLASSEXW wc{sizeof(wc)};
        wc.lpfnWndProc = &App::windowProc;
        wc.hInstance = instance_;
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.lpszClassName = kOverlayClass;
        if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
            return false;

        const int x = (GetSystemMetrics(SM_CXSCREEN) - kOverlayWidth) / 2;
        const int y = GetSystemMetrics(SM_CYSCREEN) - kOverlayHeight - 90;
        overlay_ = CreateWindowExW(
            WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW,
            kOverlayClass, kAppName, WS_POPUP, x, y,
            kOverlayWidth, kOverlayHeight, nullptr, nullptr, instance_, this);
        if (!overlay_) return false;

        SetLayeredWindowAttributes(overlay_, 0, 242, LWA_ALPHA);
        if (!renderer_.initialize(overlay_)) {
            MessageBoxW(nullptr, L"Direct2D renderer initialization failed.",
                kAppName, MB_ICONERROR | MB_OK);
            return false;
        }
        ShowWindow(overlay_, SW_SHOWNOACTIVATE);
        UpdateWindow(overlay_);

        if (!input_.start(overlay_)) {
            MessageBoxW(nullptr,
                L"Could not register keyboard and mouse Raw Input devices.",
                kAppName, MB_ICONERROR | MB_OK);
            return false;
        }
        if (!settings_.create(instance_)) return false;
        if (!RegisterHotKey(overlay_, kSettingsHotkeyId,
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
        UnregisterHotKey(overlay_, kSettingsHotkeyId);
        input_.stop();
        renderer_.shutdown();
        return static_cast<int>(message.wParam);
    }

private:
    void handleRawInput(HRAWINPUT handle) {
        RAWINPUT raw{};
        if (!InputCore::read(handle, raw)) return;
        InputEvent event{};

        if (raw.header.dwType == RIM_TYPEKEYBOARD) {
            const RAWKEYBOARD& key = raw.data.keyboard;
            event.type = (key.Flags & RI_KEY_BREAK)
                ? InputEvent::Type::KeyUp : InputEvent::Type::KeyDown;
            event.label = InputCore::keyName(key);
        } else if (raw.header.dwType == RIM_TYPEMOUSE) {
            const USHORT flags = raw.data.mouse.usButtonFlags;
            if (flags & RI_MOUSE_WHEEL) {
                event.type = InputEvent::Type::Wheel;
                const SHORT delta = static_cast<SHORT>(raw.data.mouse.usButtonData);
                event.label = delta > 0 ? L"Wheel Up" : L"Wheel Down";
            } else if (flags & RI_MOUSE_LEFT_BUTTON_DOWN) {
                event.type = InputEvent::Type::MouseDown;
                event.label = L"Left Click";
            } else if (flags & RI_MOUSE_RIGHT_BUTTON_DOWN) {
                event.type = InputEvent::Type::MouseDown;
                event.label = L"Right Click";
            } else if (flags & RI_MOUSE_MIDDLE_BUTTON_DOWN) {
                event.type = InputEvent::Type::MouseDown;
                event.label = L"Middle Click";
            } else if (flags & (RI_MOUSE_LEFT_BUTTON_UP |
                                RI_MOUSE_RIGHT_BUTTON_UP |
                                RI_MOUSE_MIDDLE_BUTTON_UP)) {
                event.type = InputEvent::Type::MouseUp;
                event.label = L"Mouse Release";
            } else {
                // Mouse movement is coalesced/ignored in Stage 1.
                return;
            }
        } else {
            return;
        }

        engine_.process(std::move(event));
        SetTimer(overlay_, 2, 16, nullptr);
        InvalidateRect(overlay_, nullptr, FALSE);
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

        switch (message) {
        case WM_INPUT:
            self->handleRawInput(reinterpret_cast<HRAWINPUT>(lParam));
            // Required cleanup for foreground WM_INPUT messages.
            return DefWindowProcW(hwnd, message, wParam, lParam);
        case WM_HOTKEY:
            if (wParam == kSettingsHotkeyId) self->settings_.show();
            return 0;
        case WM_PAINT: {
            PAINTSTRUCT paint{};
            BeginPaint(hwnd, &paint);
            self->renderer_.render(self->engine_.history().events());
            EndPaint(hwnd, &paint);
            return 0;
        }
        case WM_SIZE:
            self->renderer_.resize(LOWORD(lParam), HIWORD(lParam));
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        case WM_TIMER:
            if (wParam == 2) {
                const auto& events = self->engine_.history().events();
                if (events.empty() ||
                    GetTickCount() - events.front().timestamp > 1150) {
                    KillTimer(hwnd, 2);
                } else {
                    InvalidateRect(hwnd, nullptr, FALSE);
                }
            }
            return 0;
        case WM_DISPLAYCHANGE:
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        }
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }

    HINSTANCE instance_{};
    HWND overlay_{};
    InputCore input_;
    EventEngine engine_;
    SettingsWindow settings_;
    Renderer renderer_;
};
} // namespace keyviz

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int) {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    keyviz::App app(instance);
    if (!app.initialize()) return 1;
    return app.run();
}
