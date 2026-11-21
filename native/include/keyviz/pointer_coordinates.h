#pragma once

#include <windows.h>

#include <string>

namespace keyviz {

inline std::wstring signedCoordinate(LONG value) {
    return (value >= 0 ? L"+" : L"") + std::to_wstring(value);
}

inline std::wstring formatPointerCoordinates(
    LONG screenX, LONG screenY, const RECT& monitorBounds) {
    const LONG localX = screenX - monitorBounds.left;
    const LONG localY = screenY - monitorBounds.top;
    return L"DESKTOP  X " + signedCoordinate(screenX) +
        L"  Y " + signedCoordinate(screenY) +
        L"    DISPLAY  X " + signedCoordinate(localX) +
        L"  Y " + signedCoordinate(localY);
}

} // namespace keyviz
