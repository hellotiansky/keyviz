#include "keyviz/pointer_coordinates.h"

#include <cassert>

int main() {
    const RECT leftMonitor{-1920, 0, 0, 1080};
    const auto leftEdge = keyviz::formatPointerCoordinates(-1920, 240, leftMonitor);
    assert(leftEdge ==
        L"DESKTOP  X -1920  Y +240    DISPLAY  X +0  Y +240");

    const auto rightEdge = keyviz::formatPointerCoordinates(-1, 1079, leftMonitor);
    assert(rightEdge ==
        L"DESKTOP  X -1  Y +1079    DISPLAY  X +1919  Y +1079");

    const RECT primaryMonitor{0, 0, 1920, 1080};
    const auto primary = keyviz::formatPointerCoordinates(800, 600, primaryMonitor);
    assert(primary ==
        L"DESKTOP  X +800  Y +600    DISPLAY  X +800  Y +600");

    const RECT upperMonitor{0, -1200, 2560, 0};
    const auto upper = keyviz::formatPointerCoordinates(640, -1200, upperMonitor);
    assert(upper ==
        L"DESKTOP  X +640  Y -1200    DISPLAY  X +640  Y +0");
    return 0;
}
