#include "keyviz/input_core.h"
#include "keyviz/keyboard_translation.h"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <string>

using keyviz::InputCore;
using keyviz::InputEvent;

namespace {
RAWINPUT keyboardInput(USHORT virtualKey, USHORT makeCode, USHORT flags = 0) {
    RAWINPUT raw{};
    raw.header.dwType = RIM_TYPEKEYBOARD;
    raw.data.keyboard.MakeCode = makeCode;
    raw.data.keyboard.Flags = flags;
    raw.data.keyboard.VKey = virtualKey;
    return raw;
}

InputEvent decodeOne(USHORT virtualKey, USHORT makeCode, USHORT flags = 0) {
    const auto events = InputCore::decode(keyboardInput(virtualKey, makeCode, flags));
    assert(events.size() == 1);
    return events.front();
}

void leftAndRightControlAreDistinguished() {
    const auto left = decodeOne(VK_CONTROL, 0x1D);
    assert(left.virtualKey == VK_LCONTROL);
    assert(left.label == L"Left Ctrl");

    const auto right = decodeOne(VK_CONTROL, 0x1D, RI_KEY_E0);
    assert(right.virtualKey == VK_RCONTROL);
    assert(right.label == L"Right Ctrl");

    decodeOne(VK_CONTROL, 0x1D, RI_KEY_BREAK | RI_KEY_E0);
    decodeOne(VK_CONTROL, 0x1D, RI_KEY_BREAK);
}

void chordUsesTrackedModifiers() {
    decodeOne(VK_CONTROL, 0x1D);
    const auto key = decodeOne('A', 0x1E);
    assert(key.chordLabel.find(L"Ctrl + ") == 0);

    decodeOne('A', 0x1E, RI_KEY_BREAK);
    decodeOne(VK_CONTROL, 0x1D, RI_KEY_BREAK);
}

void altGrTracksRightAltAndControlTogether() {
    decodeOne(VK_CONTROL, 0x1D);
    decodeOne(VK_MENU, 0x38, RI_KEY_E0);
    const auto key = decodeOne('Q', 0x10);
    assert(key.chordLabel.find(L"Ctrl + Alt + ") == 0);

    decodeOne('Q', 0x10, RI_KEY_BREAK);
    decodeOne(VK_MENU, 0x38, RI_KEY_BREAK | RI_KEY_E0);
    decodeOne(VK_CONTROL, 0x1D, RI_KEY_BREAK);
}

void rightShiftAndRightAltAreTrackedInChords() {
    const auto rightShift = decodeOne(VK_SHIFT, 0x36);
    assert(rightShift.virtualKey == VK_RSHIFT);
    assert(rightShift.label == L"Right Shift");

    const auto rightAlt = decodeOne(VK_MENU, 0x38, RI_KEY_E0);
    assert(rightAlt.virtualKey == VK_RMENU);
    assert(rightAlt.label == L"Right Alt");

    const auto key = decodeOne('A', 0x1E);
    assert(key.chordLabel.find(L"Shift + Alt + ") == 0);

    decodeOne('A', 0x1E, RI_KEY_BREAK);
    decodeOne(VK_MENU, 0x38, RI_KEY_BREAK | RI_KEY_E0);
    decodeOne(VK_SHIFT, 0x36, RI_KEY_BREAK);
}

void releasingOneSideKeepsTheOtherModifierPressed() {
    decodeOne(VK_CONTROL, 0x1D);
    decodeOne(VK_CONTROL, 0x1D, RI_KEY_E0);
    decodeOne(VK_CONTROL, 0x1D, RI_KEY_BREAK | RI_KEY_E0);

    const auto stillControlled = decodeOne('B', 0x30);
    assert(stillControlled.chordLabel.find(L"Ctrl + ") == 0);

    decodeOne('B', 0x30, RI_KEY_BREAK);
    decodeOne(VK_CONTROL, 0x1D, RI_KEY_BREAK);
    const auto noLongerControlled = decodeOne('C', 0x2E);
    assert(noLongerControlled.chordLabel.find(L"Ctrl + ") != 0);
    decodeOne('C', 0x2E, RI_KEY_BREAK);
}

void resettingKeyboardStateClearsStaleModifiers() {
    decodeOne(VK_CONTROL, 0x1D);
    InputCore::resetKeyboardState();

    const auto key = decodeOne('D', 0x20);
    assert(key.chordLabel.find(L"Ctrl + ") != 0);
    decodeOne('D', 0x20, RI_KEY_BREAK);
}

void stoppingCaptureClearsTrackedModifiers() {
    InputCore input;
    decodeOne(VK_CONTROL, 0x1D);
    input.stop();

    const auto key = decodeOne('E', 0x12);
    assert(key.chordLabel.find(L"Ctrl + ") != 0);
    decodeOne('E', 0x12, RI_KEY_BREAK);
}

void translatedLabelsDistinguishTextDeadKeysAndFallbacks() {
    const wchar_t ordinary[] = L"e";
    assert(keyviz::translatedKeyLabel(1, ordinary, 1, L"Key E") == L"e");

    const wchar_t accent[] = L"^";
    assert(keyviz::translatedKeyLabel(-1, accent, 1, L"Dead key") == L"^ (dead)");

    const wchar_t nonPrintable[] = {L'\n'};
    assert(keyviz::translatedKeyLabel(1, nonPrintable, 1, L"Enter") == L"Enter");
    assert(keyviz::translatedKeyLabel(0, ordinary, 1, L"Key E") == L"Key E");
    assert(keyviz::translatedKeyLabel(-2, accent, 1, L"Dead key") == L"Dead key");
    assert(keyviz::translatedKeyLabel(1, nullptr, 1, L"Missing") == L"Missing");
    assert(keyviz::translatedKeyLabel(2, ordinary, 1, L"Too long") == L"Too long");
}
} // namespace

int main() {
    leftAndRightControlAreDistinguished();
    chordUsesTrackedModifiers();
    altGrTracksRightAltAndControlTogether();
    rightShiftAndRightAltAreTrackedInChords();
    releasingOneSideKeepsTheOtherModifierPressed();
    resettingKeyboardStateClearsStaleModifiers();
    stoppingCaptureClearsTrackedModifiers();
    translatedLabelsDistinguishTextDeadKeysAndFallbacks();
    return 0;
}
