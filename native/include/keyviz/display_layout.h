#pragma once

#include <windows.h>

#include <algorithm>

namespace keyviz {

struct OverlayBounds {
    int x{};
    int y{};
    int width{};
    int height{};
};

inline OverlayBounds calculateOverlayBounds(const RECT& work, UINT dpi) noexcept {
    dpi = std::max<UINT>(96, dpi);
    const int scaleDpi = static_cast<int>(dpi);
    const int workWidth = work.right - work.left;
    const int workHeight = work.bottom - work.top;
    const int logicalWorkWidth = MulDiv(workWidth, 96, scaleDpi);
    const int logicalWorkHeight = MulDiv(workHeight, 96, scaleDpi);

    constexpr int logicalWidth = 580;
    constexpr int logicalHeight = 156;
    constexpr int logicalBottomMargin = 36;
    const int width = MulDiv(std::max(1, std::min(logicalWidth,
        logicalWorkWidth - 40)), scaleDpi, 96);
    const int height = MulDiv(std::max(1, std::min(logicalHeight,
        logicalWorkHeight - 20)), scaleDpi, 96);
    const int margin = MulDiv(logicalBottomMargin, scaleDpi, 96);

    return {
        work.left + (workWidth - width) / 2,
        work.bottom - height - margin,
        width,
        height
    };
}

} // namespace keyviz
