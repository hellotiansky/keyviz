#include "keyviz/event_engine.h"

#include <windows.h>
#include <utility>

namespace keyviz {

void EventEngine::process(InputEvent event) {
    if (!filter_.allow(event)) return;
    event.timestamp = GetTickCount();
    history_.append(std::move(event));
}

} // namespace keyviz
