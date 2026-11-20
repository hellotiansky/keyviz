#pragma once

#include <windows.h>
#include <cstdlib>
#include <filesystem>
#include <string>

namespace keyviz {

struct FilterSettings {
    bool keyboard{true};
    bool mouseButtons{true};
    bool mouseWheel{true};
    bool mouseMove{true};
};

class SettingsStore {
public:
    FilterSettings load() const {
        const auto path = settingsPath();
        FilterSettings settings;
        settings.keyboard = readBool(path, L"Keyboard", true);
        settings.mouseButtons = readBool(path, L"MouseButtons", true);
        settings.mouseWheel = readBool(path, L"MouseWheel", true);
        settings.mouseMove = readBool(path, L"MouseMove", true);
        return settings;
    }

    bool save(const FilterSettings& settings) const {
        const auto path = settingsPath();
        if (path.empty()) return false;
        const auto write = [&path](const wchar_t* key, bool value) {
            return WritePrivateProfileStringW(L"Filters", key,
                value ? L"1" : L"0", path.c_str()) != FALSE;
        };
        return write(L"Keyboard", settings.keyboard) &&
            write(L"MouseButtons", settings.mouseButtons) &&
            write(L"MouseWheel", settings.mouseWheel) &&
            write(L"MouseMove", settings.mouseMove);
    }

private:
    static std::wstring settingsPath() {
        wchar_t* value = nullptr;
        size_t length = 0;
        if (_wdupenv_s(&value, &length, L"LOCALAPPDATA") != 0 || !value)
            return {};
        std::filesystem::path directory(value);
        std::free(value);
        directory /= L"Keyviz";
        std::error_code error;
        std::filesystem::create_directories(directory, error);
        if (error) return {};
        return (directory / L"settings.ini").wstring();
    }

    static bool readBool(const std::wstring& path,
        const wchar_t* key, bool fallback) {
        if (path.empty()) return fallback;
        return GetPrivateProfileIntW(L"Filters", key,
            fallback ? 1 : 0, path.c_str()) != 0;
    }
};

} // namespace keyviz
