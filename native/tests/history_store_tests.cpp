#include "keyviz/history_store.h"
#include "keyviz/history_persistence.h"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstddef>
#include <utility>
#include <filesystem>
#include <chrono>

using keyviz::HistoryStore;
using keyviz::InputEvent;

namespace {
InputEvent keyEvent(USHORT virtualKey, DWORD timestamp, bool repeat = false) {
    InputEvent event{};
    event.type = InputEvent::Type::KeyDown;
    event.label = L"key";
    event.virtualKey = virtualKey;
    event.timestamp = timestamp;
    event.isRepeat = repeat;
    return event;
}

void capacityIsBoundedAndCanBeChanged() {
    HistoryStore history(2);
    history.append(keyEvent('A', 1));
    history.append(keyEvent('B', 2));
    history.append(keyEvent('C', 3));

    assert(history.events().size() == 2);
    assert(history.events().front().virtualKey == 'C');
    assert(history.events().back().virtualKey == 'B');

    history.setCapacity(1);
    assert(history.capacity() == 1);
    assert(history.events().size() == 1);
    assert(history.events().front().virtualKey == 'C');

    history.setCapacity(0);
    assert(history.events().empty());
    history.append(keyEvent('D', 4));
    assert(history.events().empty());
}

void mouseMovementSamplesAreCoalesced() {
    HistoryStore history(4);
    InputEvent first{};
    first.type = InputEvent::Type::MouseMove;
    first.screenX = 10;
    first.screenY = 20;
    history.append(first);

    InputEvent second = first;
    second.screenX = 30;
    second.screenY = 40;
    history.append(second);

    assert(history.events().size() == 1);
    assert(history.events().front().screenX == 30);
    assert(history.events().front().screenY == 40);
}

void repeatedKeyDownsAreCoalesced() {
    HistoryStore history(4);
    history.append(keyEvent('A', 1));
    history.append(keyEvent('A', 2, true));

    assert(history.events().size() == 1);
    assert(history.events().front().repeatCount == 2);
    assert(history.events().front().timestamp == 2);
    assert(history.events().front().isRepeat);
}

void persistenceRoundTripsEventsAndHonorsCapacity() {
    const auto path = (std::filesystem::temp_directory_path() /
        (L"keyviz-history-test-" + std::to_wstring(GetCurrentProcessId()) + L".bin")).wstring();
    HistoryStore original(4);
    auto newest = keyEvent('B', GetTickCount());
    newest.label = L"Ctrl + B";
    newest.chordLabel = L"Ctrl+B";
    newest.wallClockMs = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
    original.append(keyEvent('A', 10));
    original.append(newest);

    assert(keyviz::HistoryPersistence::save(path, original));
    HistoryStore restored(1);
    assert(keyviz::HistoryPersistence::load(path, restored));
    assert(restored.events().size() == 1);
    assert(restored.events().front().virtualKey == 'B');
    assert(restored.events().front().label == L"Ctrl + B");
    assert(restored.events().front().chordLabel == L"Ctrl+B");
    std::error_code error;
    std::filesystem::remove(path, error);
}

void clearRemovesAllEvents() {
    HistoryStore history(4);
    history.append(keyEvent('A', 1));
    history.clear();
    assert(history.events().empty());
}
} // namespace

int main() {
    capacityIsBoundedAndCanBeChanged();
    mouseMovementSamplesAreCoalesced();
    repeatedKeyDownsAreCoalesced();
    clearRemovesAllEvents();
    persistenceRoundTripsEventsAndHonorsCapacity();
    return 0;
}
