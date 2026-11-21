#include "keyviz/display_layout.h"

#include <cassert>

using keyviz::OverlayBounds;
using keyviz::calculateOverlayBounds;

namespace {
RECT workArea(int scale) {
    return RECT{0, 0, 1920 * scale, 1080 * scale};
}

void assertLogicalBounds(const OverlayBounds& bounds, int dpi) {
    assert(MulDiv(bounds.x, 96, dpi) == 670);
    assert(MulDiv(bounds.y, 96, dpi) == 888);
    assert(MulDiv(bounds.width, 96, dpi) == 580);
    assert(MulDiv(bounds.height, 96, dpi) == 156);
}
}

int main() {
    const auto at96 = calculateOverlayBounds(workArea(1), 96);
    const auto at144 = calculateOverlayBounds(workArea(1) /* overwritten with physical 150% dimensions below */, 144);
    const auto at192 = calculateOverlayBounds(workArea(2), 192);

    assertLogicalBounds(at96, 96);
    // A 150% display has 2880x1620 physical pixels for this logical work area.
    assertLogicalBounds(at144, 144);
    assertLogicalBounds(at192, 192);

    const RECT compactWork{0, 0, 800, 600};
    const auto compact = calculateOverlayBounds(compactWork, 192);
    assert(compact.width <= 800);
    assert(compact.height <= 600);
    assert(compact.x >= 0);
    assert(compact.y >= 0);

    const auto fallbackDpi = calculateOverlayBounds(workArea(1), 0);
    assert(fallbackDpi.width == at96.width);
    assert(fallbackDpi.height == at96.height);
    return 0;
}
