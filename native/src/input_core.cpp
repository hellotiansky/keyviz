#include "keyviz/input_core.h"
#include <hidusage.h>
#include <cwctype>
#include <iterator>
#include <utility>
namespace keyviz {
POINT InputCore::lastMousePoint_{};
bool InputCore::haveMousePoint_ = false;
DWORD InputCore::lastMouseMoveTick_ = 0;
std::array<bool, 256> InputCore::pressedKeys_{};
bool InputCore::start(HWND target) noexcept {
    RAWINPUTDEVICE devices[2]{};
    devices[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
    devices[0].usUsage = HID_USAGE_GENERIC_KEYBOARD;
    devices[0].dwFlags = RIDEV_INPUTSINK;
    devices[0].hwndTarget = target;
    devices[1].usUsagePage = HID_USAGE_PAGE_GENERIC;
    devices[1].usUsage = HID_USAGE_GENERIC_MOUSE;
    devices[1].dwFlags = RIDEV_INPUTSINK;
    devices[1].hwndTarget = target;
    registered_ = RegisterRawInputDevices(devices, static_cast<UINT>(std::size(devices)), sizeof(RAWINPUTDEVICE)) == TRUE;
    if (registered_) { haveMousePoint_ = false; lastMouseMoveTick_ = 0; pressedKeys_.fill(false); }
    return registered_;
}
void InputCore::stop() noexcept {
    if (!registered_) return;
    RAWINPUTDEVICE devices[2]{};
    devices[0].usUsagePage = HID_USAGE_PAGE_GENERIC; devices[0].usUsage = HID_USAGE_GENERIC_KEYBOARD; devices[0].dwFlags = RIDEV_REMOVE;
    devices[1].usUsagePage = HID_USAGE_PAGE_GENERIC; devices[1].usUsage = HID_USAGE_GENERIC_MOUSE; devices[1].dwFlags = RIDEV_REMOVE;
    RegisterRawInputDevices(devices, static_cast<UINT>(std::size(devices)), sizeof(RAWINPUTDEVICE));
    registered_ = false; pressedKeys_.fill(false);
}
bool InputCore::read(HRAWINPUT handle, RAWINPUT& raw) noexcept {
    UINT size = sizeof(raw);
    return GetRawInputData(handle, RID_INPUT, &raw, &size, sizeof(RAWINPUTHEADER)) != static_cast<UINT>(-1);
}
USHORT InputCore::normalizedVirtualKey(const RAWKEYBOARD& key) noexcept {
    const bool extended = (key.Flags & RI_KEY_E0) != 0;
    if (key.VKey == VK_SHIFT) return key.MakeCode == 0x36 ? VK_RSHIFT : VK_LSHIFT;
    if (key.VKey == VK_CONTROL) return extended ? VK_RCONTROL : VK_LCONTROL;
    if (key.VKey == VK_MENU) return extended ? VK_RMENU : VK_LMENU;
    return key.VKey;
}
bool InputCore::isModifierKey(USHORT vk) noexcept {
    return vk == VK_LCONTROL || vk == VK_RCONTROL ||
        vk == VK_LSHIFT || vk == VK_RSHIFT ||
        vk == VK_LMENU || vk == VK_RMENU ||
        vk == VK_LWIN || vk == VK_RWIN;
}
std::wstring InputCore::chordName(USHORT vk, const std::wstring& keyLabel) {
    if (isModifierKey(vk)) return keyLabel;
    std::wstring chord;
    const auto add = [&chord](const wchar_t* name) {
        if (!chord.empty()) chord += L" + ";
        chord += name;
    };
    if (pressedKeys_[VK_LCONTROL] || pressedKeys_[VK_RCONTROL]) add(L"Ctrl");
    if (pressedKeys_[VK_LSHIFT] || pressedKeys_[VK_RSHIFT]) add(L"Shift");
    if (pressedKeys_[VK_LMENU] || pressedKeys_[VK_RMENU]) add(L"Alt");
    if (pressedKeys_[VK_LWIN] || pressedKeys_[VK_RWIN]) add(L"Win");
    if (!chord.empty()) chord += L" + ";
    chord += keyLabel;
    return chord;
}
std::wstring InputCore::localizedKeyName(const RAWKEYBOARD& key) {
    const USHORT vk = normalizedVirtualKey(key);
    if (isModifierKey(vk) || vk == VK_PACKET || vk == VK_PROCESSKEY)
        return keyName(key);

    HWND foreground = GetForegroundWindow();
    DWORD threadId = foreground ? GetWindowThreadProcessId(foreground, nullptr) : 0;
    HKL layout = GetKeyboardLayout(threadId);

    BYTE state[256]{};
    for (int code = 0; code < 256; ++code) {
        if (GetAsyncKeyState(code) & 0x8000) state[code] |= 0x80;
    }
    // Preserve toggle state while taking the down-state from the physical keyboard.
    for (const int toggleKey : {VK_CAPITAL, VK_NUMLOCK, VK_SCROLL}) {
        if (GetKeyState(toggleKey) & 1) state[toggleKey] |= 1;
    }

    wchar_t translated[8]{};
    const UINT scanCode = key.MakeCode;
    // Flag 0x4 asks Windows not to mutate the keyboard buffer's dead-key state.
    const int length = ToUnicodeEx(vk, scanCode, state, translated,
        static_cast<int>(std::size(translated)), 0x4, layout);
    if (length > 0) {
        for (int i = 0; i < length; ++i) {
            if (!iswprint(translated[i])) return keyName(key);
        }
        return std::wstring(translated, translated + length);
    }
    // Dead keys and non-printable keys retain their descriptive virtual-key name.
    return keyName(key);
}
std::wstring InputCore::keyName(const RAWKEYBOARD& key) {
    const USHORT vk = normalizedVirtualKey(key);
    switch (vk) {
    case VK_LSHIFT: return L"Left Shift"; case VK_RSHIFT: return L"Right Shift";
    case VK_LCONTROL: return L"Left Ctrl"; case VK_RCONTROL: return L"Right Ctrl";
    case VK_LMENU: return L"Left Alt"; case VK_RMENU: return L"Right Alt";
    case VK_LWIN: return L"Left Win"; case VK_RWIN: return L"Right Win";
    default: break;
    }
    if (key.VKey == 255) return {};
    const LONG keyData = static_cast<LONG>((key.MakeCode << 16) | ((key.Flags & RI_KEY_E0) ? (1L << 24) : 0));
    wchar_t name[128]{};
    if (GetKeyNameTextW(keyData, name, static_cast<int>(std::size(name))) > 0) return name;
    return L"Key " + std::to_wstring(vk);
}
std::vector<InputEvent> InputCore::decode(const RAWINPUT& raw) {
    std::vector<InputEvent> events;
    if (raw.header.dwType == RIM_TYPEKEYBOARD) {
        const RAWKEYBOARD& key = raw.data.keyboard;
        if (key.VKey == 255) return events;
        const USHORT vk = normalizedVirtualKey(key);
        if (vk >= pressedKeys_.size()) return events;
        const bool keyUp = (key.Flags & RI_KEY_BREAK) != 0;
        const bool repeat = !keyUp && pressedKeys_[vk];
        pressedKeys_[vk] = !keyUp;
        InputEvent event{};
        event.type = keyUp ? InputEvent::Type::KeyUp : InputEvent::Type::KeyDown;
        event.label = localizedKeyName(key);
        event.virtualKey = vk;
        event.isRepeat = repeat;
        if (!keyUp && !event.label.empty())
            event.chordLabel = chordName(vk, event.label);
        if (!event.label.empty()) events.push_back(std::move(event));
        return events;
    }
    if (raw.header.dwType != RIM_TYPEMOUSE) return events;
    const RAWMOUSE& mouse = raw.data.mouse;
    const USHORT flags = mouse.usButtonFlags;
    const auto addButton = [&events, flags](USHORT downFlag, USHORT upFlag, const wchar_t* label) {
        if (downFlag && (flags & downFlag)) events.push_back({InputEvent::Type::MouseDown, label, 0});
        if (upFlag && (flags & upFlag)) events.push_back({InputEvent::Type::MouseUp, label, 0});
    };
    addButton(RI_MOUSE_LEFT_BUTTON_DOWN, RI_MOUSE_LEFT_BUTTON_UP, L"Left");
    addButton(RI_MOUSE_RIGHT_BUTTON_DOWN, RI_MOUSE_RIGHT_BUTTON_UP, L"Right");
    addButton(RI_MOUSE_MIDDLE_BUTTON_DOWN, RI_MOUSE_MIDDLE_BUTTON_UP, L"Middle");
    addButton(RI_MOUSE_BUTTON_4_DOWN, RI_MOUSE_BUTTON_4_UP, L"X1");
    addButton(RI_MOUSE_BUTTON_5_DOWN, RI_MOUSE_BUTTON_5_UP, L"X2");
    if (flags & RI_MOUSE_WHEEL) { const SHORT d = static_cast<SHORT>(mouse.usButtonData); events.push_back({InputEvent::Type::Wheel, d > 0 ? L"Wheel Up" : L"Wheel Down", 0}); }
    if (flags & RI_MOUSE_HWHEEL) { const SHORT d = static_cast<SHORT>(mouse.usButtonData); events.push_back({InputEvent::Type::Wheel, d > 0 ? L"Wheel Right" : L"Wheel Left", 0}); }
    if ((mouse.usFlags & MOUSE_MOVE_ABSOLUTE) || mouse.lLastX != 0 || mouse.lLastY != 0) {
        POINT point{};
        if (GetCursorPos(&point)) {
            const DWORD now = GetTickCount();
            const bool changed = !haveMousePoint_ || point.x != lastMousePoint_.x || point.y != lastMousePoint_.y;
            if (changed && (!haveMousePoint_ || now - lastMouseMoveTick_ >= 33)) {
                InputEvent move{}; move.type = InputEvent::Type::MouseMove; move.label = L"Pointer";
                move.screenX = point.x; move.screenY = point.y; events.push_back(std::move(move));
                lastMousePoint_ = point; haveMousePoint_ = true; lastMouseMoveTick_ = now;
            }
        }
    }
    return events;
}
} // namespace keyviz
