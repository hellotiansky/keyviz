#include "keyviz/input_core.h"
#include "keyviz/mouse_sampling.h"
#include "keyviz/keyboard_translation.h"
#include <hidusage.h>
#include <cwctype>
#include <iterator>
#include <unordered_map>
#include <utility>
namespace keyviz {
POINT InputCore::lastMousePoint_{};
bool InputCore::haveMousePoint_ = false;
DWORD InputCore::lastMouseMoveTick_ = 0;
std::unordered_map<HANDLE, std::array<bool, 256>> InputCore::pressedKeysByDevice_{};
bool InputCore::start(HWND target) noexcept {
    // A new capture session must never inherit key or pointer sampling state.
    resetKeyboardState();
    haveMousePoint_ = false;
    lastMousePoint_ = {};
    lastMouseMoveTick_ = 0;

    RAWINPUTDEVICE devices[2]{};
    devices[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
    devices[0].usUsage = HID_USAGE_GENERIC_KEYBOARD;
    devices[0].dwFlags = RIDEV_INPUTSINK | RIDEV_DEVNOTIFY;
    devices[0].hwndTarget = target;
    devices[1].usUsagePage = HID_USAGE_PAGE_GENERIC;
    devices[1].usUsage = HID_USAGE_GENERIC_MOUSE;
    devices[1].dwFlags = RIDEV_INPUTSINK | RIDEV_DEVNOTIFY;
    devices[1].hwndTarget = target;
    registered_ = RegisterRawInputDevices(devices, static_cast<UINT>(std::size(devices)), sizeof(RAWINPUTDEVICE)) == TRUE;
    if (!registered_) resetKeyboardState();
    return registered_;
}
void InputCore::stop() noexcept {
    if (registered_) {
        RAWINPUTDEVICE devices[2]{};
        devices[0].usUsagePage = HID_USAGE_PAGE_GENERIC; devices[0].usUsage = HID_USAGE_GENERIC_KEYBOARD; devices[0].dwFlags = RIDEV_REMOVE;
        devices[1].usUsagePage = HID_USAGE_PAGE_GENERIC; devices[1].usUsage = HID_USAGE_GENERIC_MOUSE; devices[1].dwFlags = RIDEV_REMOVE;
        RegisterRawInputDevices(devices, static_cast<UINT>(std::size(devices)), sizeof(RAWINPUTDEVICE));
    }
    registered_ = false;
    resetKeyboardState();
    haveMousePoint_ = false;
    lastMousePoint_ = {};
    lastMouseMoveTick_ = 0;
}
void InputCore::resetKeyboardState() noexcept {
    pressedKeysByDevice_.clear();
}

bool InputCore::isKeyPressed(USHORT virtualKey) noexcept {
    for (const auto& [device, keys] : pressedKeysByDevice_) {
        (void)device;
        if (virtualKey < keys.size() && keys[virtualKey]) return true;
    }
    return false;
}

void InputCore::handleDeviceChange(WPARAM change, HANDLE device) noexcept {
    if (change != GIDC_REMOVAL) return;

    // Only keyboard packets create entries in pressedKeysByDevice_. Remove
    // this handle directly: querying device metadata during removal is
    // unreliable because Windows may already have invalidated the handle.
    const auto it = pressedKeysByDevice_.find(device);
    if (it != pressedKeysByDevice_.end()) pressedKeysByDevice_.erase(it);
}

bool InputCore::read(HRAWINPUT handle, RAWINPUT& raw) noexcept {
    raw = {};
    UINT size = sizeof(raw);
    const UINT copied = GetRawInputData(
        handle, RID_INPUT, &raw, &size, sizeof(RAWINPUTHEADER));
    if (copied == static_cast<UINT>(-1) ||
        copied != size ||
        copied > sizeof(raw) ||
        copied < sizeof(RAWINPUTHEADER)) {
        return false;
    }

    // The RAWINPUT union is sized for its largest member (HID), while
    // keyboard and mouse packets have shorter valid payloads. Validate the
    // member selected by dwType before decode() accesses it.
    switch (raw.header.dwType) {
    case RIM_TYPEKEYBOARD:
        return copied >= sizeof(RAWINPUTHEADER) + sizeof(RAWKEYBOARD);
    case RIM_TYPEMOUSE:
        return copied >= sizeof(RAWINPUTHEADER) + sizeof(RAWMOUSE);
    default:
        return false;
    }
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
    if (isKeyPressed(VK_LCONTROL) || isKeyPressed(VK_RCONTROL)) add(L"Ctrl");
    if (isKeyPressed(VK_LSHIFT) || isKeyPressed(VK_RSHIFT)) add(L"Shift");
    if (isKeyPressed(VK_LMENU) || isKeyPressed(VK_RMENU)) add(L"Alt");
    if (isKeyPressed(VK_LWIN) || isKeyPressed(VK_RWIN)) add(L"Win");
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
    // Use the Raw Input state tracked by this process rather than sampling the
    // foreground thread's asynchronous state. This keeps background capture and
    // AltGr/left-right modifier translation consistent with the captured stream.
    for (int code = 0; code < 256; ++code) {
        if (isKeyPressed(static_cast<USHORT>(code))) state[code] |= 0x80;
    }
    state[VK_SHIFT] = (state[VK_LSHIFT] || state[VK_RSHIFT]) ? 0x80 : 0;
    state[VK_CONTROL] = (state[VK_LCONTROL] || state[VK_RCONTROL]) ? 0x80 : 0;
    state[VK_MENU] = (state[VK_LMENU] || state[VK_RMENU]) ? 0x80 : 0;
    // Preserve toggle state from Windows while keeping physical modifiers local.
    for (const int toggleKey : {VK_CAPITAL, VK_NUMLOCK, VK_SCROLL}) {
        if (GetKeyState(toggleKey) & 1) state[toggleKey] |= 1;
    }

    wchar_t translated[8]{};
    // Preserve either extended prefix (E0 or E1) in the scan-code high byte.
    const UINT scanCode = keyboardTranslationScanCode(
        key.MakeCode, (key.Flags & RI_KEY_E0) != 0,
        (key.Flags & RI_KEY_E1) != 0);
    // Flag 0x4 asks Windows not to mutate the keyboard buffer's dead-key state.
    const int length = ToUnicodeEx(vk, scanCode, state, translated,
        static_cast<int>(std::size(translated)), 0x4, layout);
    // Keep normal text, dead-key accents and non-printable fallbacks in a
    // small pure helper so their formatting can be covered by CTest.
    return translatedKeyLabel(length, translated, std::size(translated), keyName(key));
}
std::wstring InputCore::keyName(const RAWKEYBOARD& key) {
    const USHORT vk = normalizedVirtualKey(key);
    switch (vk) {
    case VK_LSHIFT: return L"Left Shift"; case VK_RSHIFT: return L"Right Shift";
    case VK_LCONTROL: return L"Left Ctrl"; case VK_RCONTROL: return L"Right Ctrl";
    case VK_LMENU: return L"Left Alt"; case VK_RMENU: return L"Right Alt";
    case VK_LWIN: return L"Left Win"; case VK_RWIN: return L"Right Win";
    // Pause uses the E1-prefixed 0x45 sequence, which GetKeyNameTextW's
    // E0-only extended-key bit cannot represent reliably.
    case VK_PAUSE: return L"Pause";
    case VK_SNAPSHOT: return L"Print Screen";
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
        if (key.MakeCode == KEYBOARD_OVERRUN_MAKE_CODE) {
            // The device reports lost scan codes; its held-key snapshot is no
            // longer trustworthy, so discard it rather than leave stuck keys.
            pressedKeysByDevice_.erase(raw.header.hDevice);
            return events;
        }
        if (key.VKey == 255) return events;
        const USHORT vk = normalizedVirtualKey(key);
        if (vk >= 256) return events;
        const bool keyUp = (key.Flags & RI_KEY_BREAK) != 0;
        auto& pressedKeys = pressedKeysByDevice_[raw.header.hDevice];
        const bool repeat = !keyUp && pressedKeys[vk];
        pressedKeys[vk] = !keyUp;
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
    POINT cursorPoint{};
    const bool haveCursorPoint = GetCursorPos(&cursorPoint) != FALSE;
    const auto appendPointerEvent = [&events, &cursorPoint, haveCursorPoint](
        InputEvent::Type type, const wchar_t* label) {
        InputEvent event{};
        event.type = type;
        event.label = label;
        if (haveCursorPoint) {
            event.screenX = cursorPoint.x;
            event.screenY = cursorPoint.y;
        }
        events.push_back(std::move(event));
    };
    const auto addButton = [&appendPointerEvent, flags](
        USHORT downFlag, USHORT upFlag, const wchar_t* label) {
        if (downFlag && (flags & downFlag))
            appendPointerEvent(InputEvent::Type::MouseDown, label);
        if (upFlag && (flags & upFlag))
            appendPointerEvent(InputEvent::Type::MouseUp, label);
    };
    addButton(RI_MOUSE_LEFT_BUTTON_DOWN, RI_MOUSE_LEFT_BUTTON_UP, L"Left");
    addButton(RI_MOUSE_RIGHT_BUTTON_DOWN, RI_MOUSE_RIGHT_BUTTON_UP, L"Right");
    addButton(RI_MOUSE_MIDDLE_BUTTON_DOWN, RI_MOUSE_MIDDLE_BUTTON_UP, L"Middle");
    addButton(RI_MOUSE_BUTTON_4_DOWN, RI_MOUSE_BUTTON_4_UP, L"X1");
    addButton(RI_MOUSE_BUTTON_5_DOWN, RI_MOUSE_BUTTON_5_UP, L"X2");
    if (flags & RI_MOUSE_WHEEL) {
        const SHORT delta = static_cast<SHORT>(mouse.usButtonData);
        appendPointerEvent(InputEvent::Type::Wheel,
            delta > 0 ? L"Wheel Up" : L"Wheel Down");
    }
    if (flags & RI_MOUSE_HWHEEL) {
        const SHORT delta = static_cast<SHORT>(mouse.usButtonData);
        appendPointerEvent(InputEvent::Type::Wheel,
            delta > 0 ? L"Wheel Right" : L"Wheel Left");
    }
    if (haveCursorPoint &&
        ((mouse.usFlags & MOUSE_MOVE_ABSOLUTE) || mouse.lLastX != 0 || mouse.lLastY != 0)) {
        const DWORD now = GetTickCount();
        if (shouldSampleMouseMove(haveMousePoint_,
                lastMousePoint_.x, lastMousePoint_.y, lastMouseMoveTick_,
                cursorPoint.x, cursorPoint.y, now)) {
            InputEvent move{};
            move.type = InputEvent::Type::MouseMove;
            move.label = L"Pointer";
            move.screenX = cursorPoint.x;
            move.screenY = cursorPoint.y;
            events.push_back(std::move(move));
            lastMousePoint_ = cursorPoint;
            haveMousePoint_ = true;
            lastMouseMoveTick_ = now;
        }
    }
    return events;
}
} // namespace keyviz
