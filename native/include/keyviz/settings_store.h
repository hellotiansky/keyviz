#pragma once

#include <windows.h>
#include <cstdlib>
#include <cstddef>
#include <filesystem>
#include <string>

namespace keyviz {

struct FilterSettings {
    bool keyboard{true};
    bool mouseButtons{true};
    bool mouseWheel{true};
    bool mouseMove{true};
    std::size_t historyCapacity{80};
    DWORD animationDurationMs{1100};
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
        const UINT capacity = path.empty() ? 80 :
            GetPrivateProfileIntW(L"History", L"Capacity", 80, path.c_str());
        settings.historyCapacity = capacity == 40 || capacity == 80 ||
            capacity == 160 || capacity == 320 ? capacity : 80;
        const UINT fadeDuration = path.empty() ? 1100 :
            GetPrivateProfileIntW(L"Animation", L"FadeDurationMs", 1100, path.c_str());
        settings.animationDurationMs = fadeDuration == 400 || fadeDuration == 800 ||
            fadeDuration == 1100 || fadeDuration == 1600 ? fadeDuration : 1100;
        return settings;
    }

    bool save(const FilterSettings& settings) const {
        const auto path = settingsPath();
        if (path.empty()) return false;
        const auto write = [&path](const wchar_t* section, const wchar_t* key,
            const std::wstring& value) {
            return WritePrivateProfileStringW(section, key,
                value.c_str(), path.c_str()) != FALSE;
        };
        return write(L"Filters", L"Keyboard", settings.keyboard ? L"1" : L"0") &&
            write(L"Filters", L"MouseButtons", settings.mouseButtons ? L"1" : L"0") &&
            write(L"Filters", L"MouseWheel", settings.mouseWheel ? L"1" : L"0") &&
            write(L"Filters", L"MouseMove", settings.mouseMove ? L"1" : L"0") &&
            write(L"History", L"Capacity", std::to_wstring(settings.historyCapacity)) &&
            write(L"Animation", L"FadeDurationMs", std::to_wstring(settings.animationDurationMs));
    }

private:
    static std::wstring settingsPath() {
        wchar_t* value = nullptr;
        size_t length = 0;
        const errno_t status = _wdupenv_s(&value, &length, L"LOCALAPPDATA");
        if (status != 0 || !value) {
            std::free(value);
            return {};
        }
        if (value[0] == L'\0') {
            std::free(value);
            return {};
        }
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
