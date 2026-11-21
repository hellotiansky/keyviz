#pragma once

#include <windows.h>

namespace keyviz {

inline const wchar_t* mouseWheelLabel(SHORT delta, bool horizontal) noexcept {
    if (delta == 0) return nullptr;
    if (horizontal)
        return delta > 0 ? L"Wheel Right" : L"Wheel Left";
    return delta > 0 ? L"Wheel Up" : L"Wheel Down";
}

} // namespace keyviz
