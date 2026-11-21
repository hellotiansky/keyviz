#include "keyviz/mouse_sampling.h"

#include <cassert>

int main() {
    using keyviz::shouldSampleMouseMove;

    // Always capture the first position after Raw Input starts.
    assert(shouldSampleMouseMove(false, 0, 0, 0, 100, 200, 10));

    // Ignore duplicate coordinates even after the sampling interval.
    assert(!shouldSampleMouseMove(true, 100, 200, 10, 100, 200, 1000));

    // Changed coordinates are rate-limited to one sample per 50 ms.
    assert(!shouldSampleMouseMove(true, 100, 200, 100, 101, 200, 149));
    assert(shouldSampleMouseMove(true, 100, 200, 100, 101, 200, 150));
    assert(shouldSampleMouseMove(true, 100, 200, 100, 500, 900, 250));

    // GetTickCount wraps after roughly 49.7 days; unsigned elapsed math remains valid.
    const DWORD nearWrap = 0xFFFFFFF0u;
    assert(!shouldSampleMouseMove(true, 1, 1, nearWrap, 2, 2, 0x00000010u));
    assert(shouldSampleMouseMove(true, 1, 1, nearWrap, 2, 2, 0x00000030u));

    return 0;
}
