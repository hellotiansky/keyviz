#pragma once

#include <windows.h>
#include <string>

namespace keyviz {

struct InputEvent {
    enum class Type { KeyDown, KeyUp, MouseDown, MouseUp, Wheel };
    Type type{};
    std::wstring label;
    DWORD timestamp{};
};

} // namespace keyviz
