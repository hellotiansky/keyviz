#include <windows.h>
#include <commdlg.h>
#include <shellapi.h>
#include <dbt.h>
#include "keyviz/input_core.h"
#include "keyviz/input_event.h"
#include "keyviz/event_engine.h"
#include "keyviz/renderer.h"
#include "keyviz/settings_store.h"
#include "keyviz/history_persistence.h"
#include "keyviz/display_layout.h"

#include <algorithm>
#include <memory>
#include <functional>
#include <deque>
#include <string>
#include <utility>
#include <vector>
#include <cwctype>
#include <fstream>

namespace keyviz {
constexpr wchar_t kAppName[] = L"Keyviz Native";
constexpr wchar_t kOverlayClass[] = L"KeyvizNativeOverlay";
constexpr wchar_t kInputClass[] = L"KeyvizNativeInput";
constexpr wchar_t kSettingsClass[] = L"KeyvizNativeSettings";
constexpr int kSettingsButtonId = 100;
constexpr int kSettingsHotkeyId = 1;
constexpr int kHistoryHotkeyId = 2;
constexpr UINT_PTR kRedrawTimerId = 2;
constexpr UINT_PTR kHistorySaveTimerId = 3;
constexpr UINT_PTR kDisplayRebuildTimerId = 4;
constexpr UINT kDisplayRebuildDelayMs = 300;
constexpr UINT kTrayCallbackMessage = WM_APP + 1;
constexpr UINT kTraySettingsCommand = 401;
constexpr UINT kTrayHistoryCommand = 402;
constexpr UINT kTrayExitCommand = 403;
constexpr UINT kTrayIconId = 1;
constexpr WORD kAppIconResourceId = 101;

class SettingsWindow {
public:
    bool create(HINSTANCE instance, const FilterSettings& initial,
        std::function<bool(const FilterSettings&)> onApply,
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
            CW_USEDEFAULT, CW_USEDEFAULT, 460, 370,
            nullptr, nullptr, instance, this);
        return hwnd_ != nullptr;
    }
    void show() const {
        if (!hwnd_) return;
        ShowWindow(hwnd_, SW_SHOW);
        SetForegroundWindow(hwnd_);
    }
    void destroy() noexcept {
        if (hwnd_ && IsWindow(hwnd_)) DestroyWindow(hwnd_);
        hwnd_ = nullptr;
    }
private:
    enum : int {
        kKeyboardFilterId = 201, kMouseButtonsFilterId = 202,
        kWheelFilterId = 203, kMouseMoveFilterId = 204,
        kApplyId = 205, kHideId = 206, kClearHistoryId = 207,
        kHistoryCapacityId = 208, kAnimationDurationId = 209
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
            CreateWindowExW(0, L"STATIC", L"Maximum events kept in history:",
                WS_CHILD | WS_VISIBLE, 22, 180, 220, 22,
                hwnd, nullptr, instance, nullptr);
            HWND capacity = CreateWindowExW(0, L"COMBOBOX", nullptr,
                WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
                250, 176, 150, 120, hwnd,
                reinterpret_cast<HMENU>(static_cast<INT_PTR>(kHistoryCapacityId)),
                instance, nullptr);
            const wchar_t* capacities[] = {L"40 events", L"80 events",
                L"160 events", L"320 events"};
            for (const auto* option : capacities)
                SendMessageW(capacity, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(option));
            const int selectedCapacity = self->settings_.historyCapacity == 40 ? 0 :
                self->settings_.historyCapacity == 160 ? 2 :
                self->settings_.historyCapacity == 320 ? 3 : 1;
            SendMessageW(capacity, CB_SETCURSEL, selectedCapacity, 0);
            CreateWindowExW(0, L"STATIC", L"Overlay fade duration:",
                WS_CHILD | WS_VISIBLE, 22, 218, 220, 22,
                hwnd, nullptr, instance, nullptr);
            HWND animation = CreateWindowExW(0, L"COMBOBOX", nullptr,
                WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
                250, 214, 150, 120, hwnd,
                reinterpret_cast<HMENU>(static_cast<INT_PTR>(kAnimationDurationId)),
                instance, nullptr);
            const wchar_t* durations[] = {L"0.4 seconds", L"0.8 seconds",
                L"1.1 seconds", L"1.6 seconds"};
            for (const auto* option : durations)
                SendMessageW(animation, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(option));
            const int selectedDuration = self->settings_.animationDurationMs == 400 ? 0 :
                self->settings_.animationDurationMs == 800 ? 1 :
                self->settings_.animationDurationMs == 1600 ? 3 : 2;
            SendMessageW(animation, CB_SETCURSEL, selectedDuration, 0);
            CreateWindowExW(0, L"BUTTON", L"Apply",
                WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 22, 270, 100, 30,
                hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kApplyId)),
                instance, nullptr);
            CreateWindowExW(0, L"BUTTON", L"Hide",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 134, 270, 100, 30,
                hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kHideId)),
                instance, nullptr);
            CreateWindowExW(0, L"BUTTON", L"Clear history",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 246, 270, 130, 30,
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
                const LRESULT capacity = SendDlgItemMessageW(hwnd, kHistoryCapacityId,
                    CB_GETCURSEL, 0, 0);
                constexpr std::size_t capacities[] = {40, 80, 160, 320};
                if (capacity >= 0 && capacity < 4)
                    next.historyCapacity = capacities[capacity];
                const LRESULT animation = SendDlgItemMessageW(hwnd, kAnimationDurationId,
                    CB_GETCURSEL, 0, 0);
                constexpr DWORD durations[] = {400, 800, 1100, 1600};
                if (animation >= 0 && animation < 4)
                    next.animationDurationMs = durations[animation];
                if (self->onApply_ && self->onApply_(next)) {
                    self->settings_ = next;
                } else {
                    CheckDlgButton(hwnd, kKeyboardFilterId,
                        self->settings_.keyboard ? BST_CHECKED : BST_UNCHECKED);
                    CheckDlgButton(hwnd, kMouseButtonsFilterId,
                        self->settings_.mouseButtons ? BST_CHECKED : BST_UNCHECKED);
                    CheckDlgButton(hwnd, kWheelFilterId,
                        self->settings_.mouseWheel ? BST_CHECKED : BST_UNCHECKED);
                    CheckDlgButton(hwnd, kMouseMoveFilterId,
                        self->settings_.mouseMove ? BST_CHECKED : BST_UNCHECKED);
                    const int capacityIndex = self->settings_.historyCapacity == 40 ? 0 :
                        self->settings_.historyCapacity == 160 ? 2 :
                        self->settings_.historyCapacity == 320 ? 3 : 1;
                    SendDlgItemMessageW(hwnd, kHistoryCapacityId, CB_SETCURSEL,
                        capacityIndex, 0);
                    const int durationIndex = self->settings_.animationDurationMs == 400 ? 0 :
                        self->settings_.animationDurationMs == 800 ? 1 :
                        self->settings_.animationDurationMs == 1600 ? 3 : 2;
                    SendDlgItemMessageW(hwnd, kAnimationDurationId, CB_SETCURSEL,
                        durationIndex, 0);
                }
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
    std::function<bool(const FilterSettings&)> onApply_;
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

    void destroy() noexcept {
        if (hwnd_ && IsWindow(hwnd_)) DestroyWindow(hwnd_);
        hwnd_ = nullptr;
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
            const wchar_t* type = eventTypeLabel(event.type);
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
    static constexpr int kExportId = 305;

    static const wchar_t* eventTypeLabel(InputEvent::Type type) {
        switch (type) {
        case InputEvent::Type::KeyDown: return L"KEY DOWN";
        case InputEvent::Type::KeyUp: return L"KEY UP";
        case InputEvent::Type::MouseDown: return L"MOUSE DOWN";
        case InputEvent::Type::MouseUp: return L"MOUSE UP";
        case InputEvent::Type::Wheel: return L"WHEEL";
        case InputEvent::Type::MouseMove: return L"MOUSE MOVE";
        }
        return L"UNKNOWN";
    }

    static std::string toUtf8(const std::wstring& value) {
        if (value.empty()) return {};
        const int size = WideCharToMultiByte(CP_UTF8, 0, value.data(),
            static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
        if (size <= 0) return {};
        std::string result(static_cast<size_t>(size), '\0');
        WideCharToMultiByte(CP_UTF8, 0, value.data(),
            static_cast<int>(value.size()), result.data(), size, nullptr, nullptr);
        return result;
    }

    static std::wstring csvEscape(const std::wstring& value) {
        std::wstring escaped = L"\"";
        for (wchar_t ch : value) {
            if (ch == L'"') escaped += L'"';
            escaped += ch;
        }
        escaped += L'"';
        return escaped;
    }

    void exportCsv() const {
        if (!getEvents_ || !hwnd_) return;
        wchar_t path[MAX_PATH] = L"keyviz-history.csv";
        OPENFILENAMEW dialog{};
        dialog.lStructSize = sizeof(dialog);
        dialog.hwndOwner = hwnd_;
        dialog.lpstrFilter = L"CSV files (*.csv)\0*.csv\0All files (*.*)\0*.*\0";
        dialog.lpstrFile = path;
        dialog.nMaxFile = MAX_PATH;
        dialog.lpstrDefExt = L"csv";
        dialog.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
        if (!GetSaveFileNameW(&dialog)) return;

        HWND search = GetDlgItem(hwnd_, kSearchId);
        HWND typeFilter = GetDlgItem(hwnd_, kTypeFilterId);
        if (!search || !typeFilter) return;
        wchar_t searchBuffer[256]{};
        GetWindowTextW(search, searchBuffer, static_cast<int>(std::size(searchBuffer)));
        const std::wstring query(searchBuffer);
        const int selectedType = static_cast<int>(SendMessageW(typeFilter, CB_GETCURSEL, 0, 0));
        const auto& events = getEvents_();
        const DWORD now = GetTickCount();

        std::ofstream output(path, std::ios::binary);
        if (!output) {
            MessageBoxW(hwnd_, L"Could not create the CSV file.", kAppName,
                MB_ICONERROR | MB_OK);
            return;
        }
        output.write("\xEF\xBB\xBF", 3);
        output << "age_ms,event_type,label,chord,repeat_count\r\n";
        for (const auto& event : events) {
            const std::wstring searchable = event.label + L" " + event.chordLabel;
            if (!matchesType(event, selectedType) || !containsInsensitive(searchable, query))
                continue;
            const std::wstring row = std::to_wstring(now - event.timestamp) + L"," +
                csvEscape(eventTypeLabel(event.type)) + L"," + csvEscape(event.label) + L"," +
                csvEscape(event.chordLabel) + L"," + std::to_wstring(event.repeatCount) + L"\r\n";
            output << toUtf8(row);
        }
        if (!output) {
            MessageBoxW(hwnd_, L"An error occurred while writing the CSV file.", kAppName,
                MB_ICONERROR | MB_OK);
            return;
        }
        MessageBoxW(hwnd_, L"History exported successfully.", kAppName, MB_ICONINFORMATION | MB_OK);
    }

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
            CreateWindowExW(0, L"BUTTON", L"Export CSV", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                520, 8, 80, 26, hwnd,
                reinterpret_cast<HMENU>(static_cast<INT_PTR>(kExportId)), instance, nullptr);
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
            if (LOWORD(wParam) == kExportId && HIWORD(wParam) == BN_CLICKED) {
                self->exportCsv();
                return 0;
            }
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
                MoveWindow(search, 68, 8, std::max(120, width - 410), 26, TRUE);
            if (HWND filter = GetDlgItem(hwnd, kTypeFilterId))
                MoveWindow(filter, std::max(300, width - 280), 8, 150, 200, TRUE);
            if (HWND exportButton = GetDlgItem(hwnd, kExportId))
                MoveWindow(exportButton, std::max(460, width - 120), 8, 120, 26, TRUE);
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
    ~App() { shutdown(); }

    bool initialize() {
        if (!registerWindowClasses()) return false;
        const auto initialFilters = settingsStore_.load();
        engine_.setFilterSettings(initialFilters);
        engine_.setHistoryCapacity(initialFilters.historyCapacity);
        historyPath_ = HistoryPersistence::defaultPath();
        if (!historyPath_.empty()) HistoryPersistence::load(historyPath_, engine_.history());

        // A single hidden top-level window owns Raw Input and the global hotkey.
        // Overlay windows are presentation-only and never register input devices.
        inputWindow_ = CreateWindowExW(
            WS_EX_TOOLWINDOW, kInputClass, kAppName, WS_POPUP,
            0, 0, 0, 0, nullptr, nullptr, instance_, this);
        if (!inputWindow_) return false;
        SetTimer(inputWindow_, kHistorySaveTimerId, 30000, nullptr);

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
                if (!settingsStore_.save(filters)) {
                    MessageBoxW(inputWindow_, L"Could not save settings. The previous settings remain active.",
                        kAppName, MB_ICONWARNING | MB_OK);
                    return false;
                }
                engine_.setFilterSettings(filters);
                engine_.setHistoryCapacity(filters.historyCapacity);
                return true;
            }, [this]() {
                if (!historyPath_.empty()) {
                    HistoryStore emptyHistory(engine_.history().capacity());
                    if (!HistoryPersistence::save(historyPath_, emptyHistory)) {
                        MessageBoxW(inputWindow_,
                            L"Could not clear saved history. Existing history remains unchanged.",
                            kAppName, MB_ICONWARNING | MB_OK);
                        return;
                    }
                }
                engine_.clearHistory();
                historyWindow_.refresh();
                KillTimer(inputWindow_, kRedrawTimerId);
                invalidateOverlays();
            })) return false;
        addTrayIcon();
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
    bool addTrayIcon() noexcept {
        NOTIFYICONDATAW icon{};
        icon.cbSize = sizeof(icon);
        icon.hWnd = inputWindow_;
        icon.uID = kTrayIconId;
        icon.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
        icon.uCallbackMessage = kTrayCallbackMessage;
        icon.hIcon = static_cast<HICON>(LoadImageW(instance_,
            MAKEINTRESOURCEW(kAppIconResourceId), IMAGE_ICON, 0, 0,
            LR_DEFAULTSIZE | LR_SHARED));
        if (!icon.hIcon) icon.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
        lstrcpynW(icon.szTip, L"Keyviz Native", static_cast<int>(std::size(icon.szTip)));
        trayIconAdded_ = Shell_NotifyIconW(NIM_ADD, &icon) != FALSE;
        return trayIconAdded_;
    }

    void removeTrayIcon() noexcept {
        if (!trayIconAdded_) return;
        NOTIFYICONDATAW icon{};
        icon.cbSize = sizeof(icon);
        icon.hWnd = inputWindow_;
        icon.uID = kTrayIconId;
        Shell_NotifyIconW(NIM_DELETE, &icon);
        trayIconAdded_ = false;
    }

    void showTrayMenu() {
        HMENU menu = CreatePopupMenu();
        if (!menu) return;
        AppendMenuW(menu, MF_STRING, kTraySettingsCommand, L"Settings");
        AppendMenuW(menu, MF_STRING, kTrayHistoryCommand, L"Input history");
        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(menu, MF_STRING, kTrayExitCommand, L"Exit Keyviz");
        POINT cursor{};
        GetCursorPos(&cursor);
        SetForegroundWindow(inputWindow_);
        TrackPopupMenu(menu, TPM_RIGHTBUTTON, cursor.x, cursor.y, 0,
            inputWindow_, nullptr);
        PostMessageW(inputWindow_, WM_NULL, 0, 0);
        DestroyMenu(menu);
    }

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
        // Create the window on its target monitor first so Windows assigns
        // that monitor's effective DPI before we calculate physical pixels.
        const int anchorX = work.left + workWidth / 2;
        const int anchorY = work.top + workHeight / 2;
        HWND hwnd = CreateWindowExW(
            WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW,
            kOverlayClass, kAppName, WS_POPUP, anchorX, anchorY, 1, 1,
            nullptr, nullptr, instance_, this);
        if (!hwnd) return false;

        const UINT dpi = GetDpiForWindow(hwnd);
        const OverlayBounds bounds = calculateOverlayBounds(work, dpi);
        if (!SetWindowPos(hwnd, HWND_TOPMOST, bounds.x, bounds.y,
                bounds.width, bounds.height, SWP_NOACTIVATE)) {
            DestroyWindow(hwnd);
            return false;
        }

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

    void scheduleDisplayRebuild() noexcept {
        if (inputWindow_)
            SetTimer(inputWindow_, kDisplayRebuildTimerId,
                kDisplayRebuildDelayMs, nullptr);
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

    void saveHistoryCheckpoint() noexcept {
        if (historyPath_.empty()) return;
        if (HistoryPersistence::save(historyPath_, engine_.history())) {
            historySaveWarningShown_ = false;
            return;
        }
        if (!historySaveWarningShown_) {
            historySaveWarningShown_ = true;
            MessageBoxW(inputWindow_,
                L"Keyviz could not save input history. Check that your user profile folder is writable and has free space.",
                kAppName, MB_ICONWARNING | MB_OK);
        }
    }

    void shutdown() noexcept {
        if (shutdown_) return;
        shutdown_ = true;
        saveHistoryCheckpoint();
        if (inputWindow_) {
            removeTrayIcon();
            KillTimer(inputWindow_, kRedrawTimerId);
            KillTimer(inputWindow_, kHistorySaveTimerId);
            KillTimer(inputWindow_, kDisplayRebuildTimerId);
            UnregisterHotKey(inputWindow_, kSettingsHotkeyId);
            UnregisterHotKey(inputWindow_, kHistoryHotkeyId);
        }
        input_.stop();
        settings_.destroy();
        historyWindow_.destroy();
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
        case kTrayCallbackMessage:
            if (isInputWindow && (lParam == WM_RBUTTONUP || lParam == WM_CONTEXTMENU))
                self->showTrayMenu();
            else if (isInputWindow && lParam == WM_LBUTTONDBLCLK)
                self->settings_.show();
            return 0;
        case WM_COMMAND:
            if (isInputWindow && LOWORD(wParam) == kTraySettingsCommand)
                self->settings_.show();
            else if (isInputWindow && LOWORD(wParam) == kTrayHistoryCommand)
                self->historyWindow_.show();
            else if (isInputWindow && LOWORD(wParam) == kTrayExitCommand)
                DestroyWindow(hwnd);
            return 0;
        case WM_INPUT:
            if (isInputWindow)
                self->handleRawInput(reinterpret_cast<HRAWINPUT>(lParam));
            // Required cleanup for foreground WM_INPUT messages.
            return DefWindowProcW(hwnd, message, wParam, lParam);
        case WM_INPUT_DEVICE_CHANGE:
            if (isInputWindow)
                InputCore::handleDeviceChange(
                    wParam, reinterpret_cast<HANDLE>(lParam));
            return 0;
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
                overlay->renderer.render(self->engine_.history().events(),
                    self->engine_.filterSettings().animationDurationMs);
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
                    GetTickCount() - events.front().timestamp >
                        self->engine_.filterSettings().animationDurationMs + 50) {
                    KillTimer(hwnd, kRedrawTimerId);
                }
                self->invalidateOverlays();
            } else if (isInputWindow && wParam == kHistorySaveTimerId) {
                self->saveHistoryCheckpoint();
            } else if (isInputWindow && wParam == kDisplayRebuildTimerId) {
                KillTimer(hwnd, kDisplayRebuildTimerId);
                self->rebuildOverlays();
            }
            return 0;
        case WM_DPICHANGED:
            if (!isInputWindow && self->findOverlay(hwnd)) {
                self->scheduleDisplayRebuild();
                return 0;
            }
            break;
        case WM_DISPLAYCHANGE:
            if (isInputWindow) self->scheduleDisplayRebuild();
            return 0;
        case WM_DEVICECHANGE:
            if (isInputWindow && wParam == DBT_DEVNODES_CHANGED) {
                self->scheduleDisplayRebuild();
                return TRUE;
            }
            break;
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
    std::wstring historyPath_;
    bool trayIconAdded_{};
    bool historySaveWarningShown_{};
    bool shutdown_{};
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
