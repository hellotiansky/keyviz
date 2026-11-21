#include "keyviz/event_engine.h"

#include <windows.h>
#include <utility>
#include <chrono>

namespace keyviz {

void EventEngine::process(InputEvent event) {
    if (!filter_.allow(event)) return;
    event.timestamp = GetTickCount();
    event.wallClockMs = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
    history_.append(std::move(event));
}

} // namespace keyviz
