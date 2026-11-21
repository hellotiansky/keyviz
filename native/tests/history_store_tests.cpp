#include "keyviz/history_store.h"
#include "keyviz/history_persistence.h"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <limits>
#include <cstddef>
#include <utility>
#include <filesystem>
#include <chrono>
#include <fstream>

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
    second.timestamp = 150;
    second.wallClockMs = 1700000000123ull;
    history.append(second);

    assert(history.events().size() == 1);
    assert(history.events().front().screenX == 30);
    assert(history.events().front().screenY == 40);
    assert(history.events().front().timestamp == 150);
    assert(history.events().front().wallClockMs == 1700000000123ull);
}

void movementCoalescingPreservesInterveningActions() {
    HistoryStore history(8);
    InputEvent move{};
    move.type = InputEvent::Type::MouseMove;
    move.screenX = -120;
    move.screenY = 40;
    history.append(move);

    InputEvent click{};
    click.type = InputEvent::Type::MouseDown;
    click.label = L"Left";
    history.append(click);

    InputEvent afterClick = move;
    afterClick.screenX = -80;
    afterClick.screenY = 60;
    history.append(afterClick);

    InputEvent latest = afterClick;
    latest.screenX = -20;
    latest.screenY = 90;
    history.append(latest);

    assert(history.events().size() == 3);
    assert(history.events()[0].type == InputEvent::Type::MouseMove);
    assert(history.events()[0].screenX == -20);
    assert(history.events()[1].type == InputEvent::Type::MouseDown);
    assert(history.events()[2].type == InputEvent::Type::MouseMove);
    assert(history.events()[2].screenX == -120);

    InputEvent wheel{};
    wheel.type = InputEvent::Type::Wheel;
    wheel.label = L"Wheel Up";
    history.append(wheel);
    InputEvent afterWheel = latest;
    afterWheel.screenX = 15;
    history.append(afterWheel);

    assert(history.events().size() == 5);
    assert(history.events()[0].type == InputEvent::Type::MouseMove);
    assert(history.events()[1].type == InputEvent::Type::Wheel);
    assert(history.events()[2].type == InputEvent::Type::MouseMove);
}

void repeatedKeyDownsAreCoalesced() {
    HistoryStore history(4);
    history.append(keyEvent('A', 1));
    auto repeat = keyEvent('A', 2, true);
    repeat.wallClockMs = 200;
    history.append(repeat);

    assert(history.events().size() == 1);
    assert(history.events().front().repeatCount == 2);
    assert(history.events().front().timestamp == 2);
    assert(history.events().front().wallClockMs == 200);
    assert(history.events().front().isRepeat);
}

void repeatCountSaturatesInsteadOfWrapping() {
    HistoryStore history(4);
    auto initial = keyEvent('Q', 1);
    initial.repeatCount = std::numeric_limits<std::uint32_t>::max();
    history.append(initial);

    history.append(keyEvent('Q', 2, true));
    assert(history.events().size() == 1);
    assert(history.events().front().repeatCount ==
        std::numeric_limits<std::uint32_t>::max());
    assert(history.events().front().timestamp == 2);
}

void repeatedKeysFromDifferentKeyboardsAreNotMerged() {
    const HANDLE firstKeyboard =
        reinterpret_cast<HANDLE>(static_cast<std::uintptr_t>(101));
    const HANDLE secondKeyboard =
        reinterpret_cast<HANDLE>(static_cast<std::uintptr_t>(102));
    HistoryStore history(8);

    auto firstDown = keyEvent('A', 1);
    firstDown.sourceDevice = firstKeyboard;
    history.append(firstDown);

    auto secondDown = keyEvent('A', 2);
    secondDown.sourceDevice = secondKeyboard;
    history.append(secondDown);

    // A repeat from keyboard one must not be attributed to keyboard two's
    // newer A press, even though the virtual key is identical.
    auto firstRepeat = keyEvent('A', 3, true);
    firstRepeat.sourceDevice = firstKeyboard;
    history.append(firstRepeat);
    assert(history.events().size() == 3);
    assert(history.events()[0].sourceDevice == firstKeyboard);
    assert(history.events()[0].repeatCount == 1);
    assert(history.events()[1].sourceDevice == secondKeyboard);
    assert(history.events()[1].repeatCount == 1);

    // Subsequent adjacent repeats from keyboard one coalesce normally.
    auto nextRepeat = keyEvent('A', 4, true);
    nextRepeat.sourceDevice = firstKeyboard;
    history.append(nextRepeat);
    assert(history.events().size() == 3);
    assert(history.events().front().sourceDevice == firstKeyboard);
    assert(history.events().front().repeatCount == 2);
    assert(history.events()[1].sourceDevice == secondKeyboard);
    assert(history.events()[1].repeatCount == 1);
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
    newest.sourceDevice = reinterpret_cast<HANDLE>(static_cast<std::uintptr_t>(303));
    original.append(keyEvent('A', 10));
    original.append(newest);

    assert(keyviz::HistoryPersistence::save(path, original));
    HistoryStore restored(1);
    assert(keyviz::HistoryPersistence::load(path, restored));
    assert(restored.events().size() == 1);
    assert(restored.events().front().virtualKey == 'B');
    assert(restored.events().front().label == L"Ctrl + B");
    assert(restored.events().front().chordLabel == L"Ctrl+B");
    // Raw Input handles are process-local runtime identity and must not survive persistence.
    assert(restored.events().front().sourceDevice == nullptr);
    std::error_code error;
    std::filesystem::remove(path, error);
}


void truncatedPersistenceDoesNotReplaceExistingHistory() {
    const auto path = (std::filesystem::temp_directory_path() /
        (L"keyviz-history-truncated-" + std::to_wstring(GetCurrentProcessId()) + L".bin")).wstring();
    {
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        constexpr char magic[] = "KVZHIST1";
        output.write(magic, sizeof(magic) - 1);
        const std::uint32_t count = 1;
        output.write(reinterpret_cast<const char*>(&count), sizeof(count));
        // The declared event record is intentionally absent.
        assert(static_cast<bool>(output));
    }

    HistoryStore existing(4);
    existing.append(keyEvent('Z', 99));
    assert(!keyviz::HistoryPersistence::load(path, existing));
    assert(existing.events().size() == 1);
    assert(existing.events().front().virtualKey == 'Z');

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
    movementCoalescingPreservesInterveningActions();
    repeatedKeyDownsAreCoalesced();
    repeatCountSaturatesInsteadOfWrapping();
    repeatedKeysFromDifferentKeyboardsAreNotMerged();
    clearRemovesAllEvents();
    persistenceRoundTripsEventsAndHonorsCapacity();
    truncatedPersistenceDoesNotReplaceExistingHistory();
    return 0;
}
