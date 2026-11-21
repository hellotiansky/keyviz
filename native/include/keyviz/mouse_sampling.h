#pragma once

#include <windows.h>

namespace keyviz {

inline constexpr DWORD kMouseMoveSampleIntervalMs = 50;

inline bool shouldSampleMouseMove(
    bool havePreviousPoint,
    LONG previousX,
    LONG previousY,
    DWORD previousTick,
    LONG currentX,
    LONG currentY,
    DWORD currentTick) noexcept {
    if (!havePreviousPoint) return true;
    if (previousX == currentX && previousY == currentY) return false;
    // Unsigned subtraction intentionally handles GetTickCount wraparound.
    return static_cast<DWORD>(currentTick - previousTick) >=
        kMouseMoveSampleIntervalMs;
}

} // namespace keyviz
