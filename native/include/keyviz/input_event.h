#pragma once
#include <windows.h>
#include <cstdint>
#include <string>
namespace keyviz {
struct InputEvent {
    enum class Type { KeyDown, KeyUp, MouseDown, MouseUp, Wheel, MouseMove };
    Type type{};
    std::wstring label;
    DWORD timestamp{};
    std::uint64_t wallClockMs{};
    LONG screenX{};
    LONG screenY{};
    USHORT virtualKey{};
    // Runtime-only Raw Input source identity; not persisted with historical events.
    HANDLE sourceDevice{};
    unsigned repeatCount{1};
    bool isRepeat{false};
    std::wstring chordLabel;
};
} // namespace keyviz
