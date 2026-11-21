#pragma once
#include "keyviz/input_event.h"
#include <array>
#include <windows.h>
#include <string>
#include <vector>
namespace keyviz {
class InputCore {
public:
    bool start(HWND target) noexcept;
    void stop() noexcept;
    static bool read(HRAWINPUT handle, RAWINPUT& raw) noexcept;
    static void handleDeviceChange(WPARAM change, HANDLE device) noexcept;
    static void resetKeyboardState() noexcept;
    static std::vector<InputEvent> decode(const RAWINPUT& raw);
    static std::wstring keyName(const RAWKEYBOARD& key);
private:
    static USHORT normalizedVirtualKey(const RAWKEYBOARD& key) noexcept;
    static bool isModifierKey(USHORT virtualKey) noexcept;
    static std::wstring chordName(USHORT virtualKey, const std::wstring& keyLabel);
    static std::wstring localizedKeyName(const RAWKEYBOARD& key);
    bool registered_{false};
    static POINT lastMousePoint_;
    static bool haveMousePoint_;
    static DWORD lastMouseMoveTick_;
    static std::array<bool, 256> pressedKeys_;
};
} // namespace keyviz
