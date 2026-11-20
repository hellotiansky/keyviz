#pragma once
#include <windows.h>
#include <string>
namespace keyviz {
struct InputEvent {
    enum class Type { KeyDown, KeyUp, MouseDown, MouseUp, Wheel, MouseMove };
    Type type{};
    std::wstring label;
    DWORD timestamp{};
    LONG screenX{};
    LONG screenY{};
    USHORT virtualKey{};
    unsigned repeatCount{1};
    bool isRepeat{false};
    std::wstring chordLabel;
};
} // namespace keyviz
