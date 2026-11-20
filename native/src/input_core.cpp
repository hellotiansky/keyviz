#include "keyviz/input_core.h"
#include <hidusage.h>
#include <iterator>
#include <utility>

namespace keyviz {

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
    return registered_;
}

void InputCore::stop() noexcept {
    if (!registered_) return;
    RAWINPUTDEVICE devices[2]{};
    devices[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
    devices[0].usUsage = HID_USAGE_GENERIC_KEYBOARD;
    devices[0].dwFlags = RIDEV_REMOVE;
    devices[1].usUsagePage = HID_USAGE_PAGE_GENERIC;
    devices[1].usUsage = HID_USAGE_GENERIC_MOUSE;
    devices[1].dwFlags = RIDEV_REMOVE;
    RegisterRawInputDevices(devices, static_cast<UINT>(std::size(devices)), sizeof(RAWINPUTDEVICE));
    registered_ = false;
}

bool InputCore::read(HRAWINPUT handle, RAWINPUT& raw) noexcept {
    UINT size = sizeof(raw);
    return GetRawInputData(handle, RID_INPUT, &raw, &size, sizeof(RAWINPUTHEADER)) != static_cast<UINT>(-1);
}

std::wstring InputCore::keyName(const RAWKEYBOARD& key) {
    if (key.VKey == 255) return {};
    const LONG keyData = static_cast<LONG>((key.MakeCode << 16) | ((key.Flags & RI_KEY_E0) ? (1L << 24) : 0));
    wchar_t name[128]{};
    if (GetKeyNameTextW(keyData, name, static_cast<int>(std::size(name))) > 0) return name;
    return L"Key " + std::to_wstring(key.VKey);
}

std::vector<InputEvent> InputCore::decode(const RAWINPUT& raw) {
    std::vector<InputEvent> events;
    if (raw.header.dwType == RIM_TYPEKEYBOARD) {
        const RAWKEYBOARD& key = raw.data.keyboard;
        if (key.VKey == 255) return events;
        InputEvent event{};
        event.type = (key.Flags & RI_KEY_BREAK) ? InputEvent::Type::KeyUp : InputEvent::Type::KeyDown;
        event.label = keyName(key);
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
    if (flags & RI_MOUSE_WHEEL) {
        const SHORT delta = static_cast<SHORT>(mouse.usButtonData);
        events.push_back({InputEvent::Type::Wheel, delta > 0 ? L"Wheel Up" : L"Wheel Down", 0});
    }
    if (flags & RI_MOUSE_HWHEEL) {
        const SHORT delta = static_cast<SHORT>(mouse.usButtonData);
        events.push_back({InputEvent::Type::Wheel, delta > 0 ? L"Wheel Right" : L"Wheel Left", 0});
    }
    return events;
}

} // namespace keyviz
