#pragma once

#include <cwctype>
#include <cstddef>
#include <string>

namespace keyviz {

inline unsigned int keyboardTranslationScanCode(
    unsigned short makeCode, bool e0, bool e1) noexcept {
    const unsigned int prefix = e1 ? 0xE100u : (e0 ? 0xE000u : 0u);
    return static_cast<unsigned int>(makeCode) | prefix;
}


inline std::wstring translatedKeyLabel(
    int translatedLength,
    const wchar_t* translated,
    std::size_t bufferCapacity,
    const std::wstring& fallback) {
    if (translatedLength == 0 || translated == nullptr)
        return fallback;

    const bool deadKey = translatedLength < 0;
    const auto count = static_cast<std::size_t>(
        deadKey ? -static_cast<long long>(translatedLength) : translatedLength);
    if (count == 0 || count > bufferCapacity)
        return fallback;

    for (std::size_t i = 0; i < count; ++i) {
        if (!iswprint(translated[i]))
            return fallback;
    }

    std::wstring label(translated, translated + count);
    if (deadKey)
        label += L" (dead)";
    return label;
}

} // namespace keyviz
