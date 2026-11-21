#include "keyviz/input_core.h"
#include "keyviz/keyboard_translation.h"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <string>

using keyviz::InputCore;
using keyviz::InputEvent;

namespace {
RAWINPUT keyboardInput(USHORT virtualKey, USHORT makeCode, USHORT flags = 0, HANDLE device = nullptr) {
    RAWINPUT raw{};
    raw.header.dwType = RIM_TYPEKEYBOARD;
    raw.header.hDevice = device;
    raw.data.keyboard.MakeCode = makeCode;
    raw.data.keyboard.Flags = flags;
    raw.data.keyboard.VKey = virtualKey;
    return raw;
}

InputEvent decodeOne(USHORT virtualKey, USHORT makeCode, USHORT flags = 0, HANDLE device = nullptr) {
    const auto events = InputCore::decode(keyboardInput(virtualKey, makeCode, flags, device));
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
    const HANDLE firstKeyboard = reinterpret_cast<HANDLE>(static_cast<std::uintptr_t>(11));
    const HANDLE secondKeyboard = reinterpret_cast<HANDLE>(static_cast<std::uintptr_t>(12));
    decodeOne(VK_CONTROL, 0x1D, 0, firstKeyboard);
    decodeOne(VK_SHIFT, 0x2A, 0, secondKeyboard);
    input.stop();

    const HANDLE nextKeyboard = reinterpret_cast<HANDLE>(static_cast<std::uintptr_t>(13));
    const auto key = decodeOne('E', 0x12, 0, nextKeyboard);
    assert(key.chordLabel.find(L"Ctrl + ") != 0);
    assert(key.chordLabel.find(L"Shift + ") != 0);
    decodeOne('E', 0x12, RI_KEY_BREAK, nextKeyboard);
}

void repeatedKeysAreTrackedIndependentlyPerKeyboard() {
    const HANDLE firstKeyboard = reinterpret_cast<HANDLE>(static_cast<std::uintptr_t>(21));
    const HANDLE secondKeyboard = reinterpret_cast<HANDLE>(static_cast<std::uintptr_t>(22));
    InputCore::resetKeyboardState();

    const auto firstDown = decodeOne('A', 0x1E, 0, firstKeyboard);
    const auto secondDown = decodeOne('A', 0x1E, 0, secondKeyboard);
    assert(!firstDown.isRepeat);
    assert(!secondDown.isRepeat);

    const auto firstRepeat = decodeOne('A', 0x1E, 0, firstKeyboard);
    const auto secondRepeat = decodeOne('A', 0x1E, 0, secondKeyboard);
    assert(firstRepeat.isRepeat);
    assert(secondRepeat.isRepeat);

    decodeOne('A', 0x1E, RI_KEY_BREAK, firstKeyboard);
    const auto firstFreshDown = decodeOne('A', 0x1E, 0, firstKeyboard);
    assert(!firstFreshDown.isRepeat);
    assert(secondRepeat.isRepeat);

    decodeOne('A', 0x1E, RI_KEY_BREAK, firstKeyboard);
    decodeOne('A', 0x1E, RI_KEY_BREAK, secondKeyboard);
    InputCore::resetKeyboardState();
}


void releasingModifierOnOneKeyboardPreservesTheOtherDevice() {
    const HANDLE firstKeyboard = reinterpret_cast<HANDLE>(static_cast<std::uintptr_t>(31));
    const HANDLE secondKeyboard = reinterpret_cast<HANDLE>(static_cast<std::uintptr_t>(32));
    InputCore::resetKeyboardState();

    decodeOne(VK_CONTROL, 0x1D, 0, firstKeyboard);
    decodeOne(VK_CONTROL, 0x1D, 0, secondKeyboard);

    // Arrival notifications must not disturb modifiers already held.
    InputCore::handleDeviceChange(GIDC_ARRIVAL,
        reinterpret_cast<HANDLE>(static_cast<std::uintptr_t>(33)));
    decodeOne(VK_CONTROL, 0x1D, RI_KEY_BREAK, firstKeyboard);

    const auto stillControlled = decodeOne('H', 0x23, 0, secondKeyboard);
    assert(stillControlled.chordLabel.find(L"Ctrl + ") == 0);

    decodeOne('H', 0x23, RI_KEY_BREAK, secondKeyboard);
    decodeOne(VK_CONTROL, 0x1D, RI_KEY_BREAK, secondKeyboard);
    const auto released = decodeOne('J', 0x24, 0, secondKeyboard);
    assert(released.chordLabel.find(L"Ctrl + ") != 0);
    decodeOne('J', 0x24, RI_KEY_BREAK, secondKeyboard);
    InputCore::resetKeyboardState();
}

void removingAndReconnectingKeyboardResetsRepeatState() {
    const HANDLE keyboard = reinterpret_cast<HANDLE>(static_cast<std::uintptr_t>(41));
    InputCore::resetKeyboardState();

    const auto initialDown = decodeOne('A', 0x1E, 0, keyboard);
    assert(!initialDown.isRepeat);

    // Removal discards the device's held-key map. A later arrival/press must
    // start a fresh sequence even if Windows reuses the opaque handle value.
    InputCore::handleDeviceChange(GIDC_REMOVAL, keyboard);
    InputCore::handleDeviceChange(GIDC_ARRIVAL, keyboard);
    const auto reconnectedDown = decodeOne('A', 0x1E, 0, keyboard);
    assert(!reconnectedDown.isRepeat);

    decodeOne('A', 0x1E, RI_KEY_BREAK, keyboard);
    InputCore::resetKeyboardState();
}

void removingOneKeyboardPreservesOtherDeviceModifiers() {
    const HANDLE firstKeyboard = reinterpret_cast<HANDLE>(static_cast<std::uintptr_t>(1));
    const HANDLE secondKeyboard = reinterpret_cast<HANDLE>(static_cast<std::uintptr_t>(2));
    InputCore::resetKeyboardState();

    decodeOne(VK_CONTROL, 0x1D, 0, firstKeyboard);
    decodeOne(VK_SHIFT, 0x2A, 0, secondKeyboard);

    // An unrelated removal must not clear either tracked keyboard.
    InputCore::handleDeviceChange(GIDC_REMOVAL,
        reinterpret_cast<HANDLE>(static_cast<std::uintptr_t>(3)));
    const auto bothModifiers = decodeOne('F', 0x21, 0, secondKeyboard);
    assert(bothModifiers.chordLabel.find(L"Ctrl + Shift + ") == 0);
    decodeOne('F', 0x21, RI_KEY_BREAK, secondKeyboard);

    // Removing the first keyboard clears only its modifier.
    InputCore::handleDeviceChange(GIDC_REMOVAL, firstKeyboard);
    const auto remainingModifier = decodeOne('G', 0x22, 0, secondKeyboard);
    assert(remainingModifier.chordLabel.find(L"Shift + ") == 0);
    assert(remainingModifier.chordLabel.find(L"Ctrl + ") != 0);

    decodeOne('G', 0x22, RI_KEY_BREAK, secondKeyboard);
    decodeOne(VK_SHIFT, 0x2A, RI_KEY_BREAK, secondKeyboard);
    InputCore::resetKeyboardState();
}
void keyboardTranslationPreservesExtendedScanCodePrefixes() {
    assert(keyviz::keyboardTranslationScanCode(0x1D, false, false) == 0x001D);
    assert(keyviz::keyboardTranslationScanCode(0x1D, true, false) == 0xE01D);
    assert(keyviz::keyboardTranslationScanCode(0x45, false, true) == 0xE145);
}

void invalidVirtualKeysAreIgnoredWithoutLosingHeldModifiers() {
    InputCore::resetKeyboardState();
    decodeOne(VK_CONTROL, 0x1D);

    const auto decodeInvalid = [](USHORT virtualKey) {
        const auto events = InputCore::decode(keyboardInput(virtualKey, 0x1D));
        assert(events.empty());
    };
    decodeInvalid(0x0100); // Outside the 256-entry virtual-key state table.
    decodeInvalid(0x00FF); // Raw Input's reserved/invalid VKey sentinel.

    const auto key = decodeOne('K', 0x25);
    assert(key.chordLabel.find(L"Ctrl + ") == 0);

    decodeOne('K', 0x25, RI_KEY_BREAK);
    decodeOne(VK_CONTROL, 0x1D, RI_KEY_BREAK);
    InputCore::resetKeyboardState();
}

void pauseAndPrintScreenUseStablePhysicalLabels() {
    const auto pause = decodeOne(VK_PAUSE, 0x45, RI_KEY_E1);
    assert(pause.label == L"Pause");

    const auto printScreen = decodeOne(VK_SNAPSHOT, 0x37, RI_KEY_E0);
    assert(printScreen.label == L"Print Screen");
}

void translatedLabelsDistinguishTextDeadKeysAndFallbacks() {
    const wchar_t ordinary[] = L"e";
    assert(keyviz::translatedKeyLabel(1, ordinary, 1, L"Key E") == L"e");

    // Unicode text must not depend on the process CRT locale.
    const wchar_t japanese[] = L"あ";
    assert(keyviz::translatedKeyLabel(1, japanese, 1, L"Key A") == L"あ");
    
    // UTF-16 surrogate pairs are valid text and must pass through unchanged.
    const wchar_t supplementary[] = {static_cast<wchar_t>(0xD83D), static_cast<wchar_t>(0xDE00)};
    assert(keyviz::translatedKeyLabel(2, supplementary, 2, L"Fallback") ==
        std::wstring(supplementary, supplementary + 2));

    const wchar_t accent[] = L"^";
    assert(keyviz::translatedKeyLabel(-1, accent, 1, L"Dead key") == L"^ (dead)");

    const wchar_t nonPrintable[] = {L'\n'};
    assert(keyviz::translatedKeyLabel(1, nonPrintable, 1, L"Enter") == L"Enter");
    const wchar_t c1Control[] = {static_cast<wchar_t>(0x0085)};
    assert(keyviz::translatedKeyLabel(1, c1Control, 1, L"Fallback") == L"Fallback");
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
    repeatedKeysAreTrackedIndependentlyPerKeyboard();
    releasingModifierOnOneKeyboardPreservesTheOtherDevice();
    removingOneKeyboardPreservesOtherDeviceModifiers();
    removingAndReconnectingKeyboardResetsRepeatState();
    translatedLabelsDistinguishTextDeadKeysAndFallbacks();
    keyboardTranslationPreservesExtendedScanCodePrefixes();
    invalidVirtualKeysAreIgnoredWithoutLosingHeldModifiers();
    pauseAndPrintScreenUseStablePhysicalLabels();
    return 0;
}
