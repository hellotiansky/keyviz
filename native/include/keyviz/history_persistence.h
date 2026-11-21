#pragma once

#include "keyviz/history_store.h"

#include <windows.h>
#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace keyviz {

class HistoryPersistence {
public:
    static std::wstring defaultPath() {
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
        return (directory / L"history.bin").wstring();
    }

    static bool save(const std::wstring& path, const HistoryStore& history) {
        if (path.empty()) return false;
        const std::filesystem::path target(path);
        const std::filesystem::path temporary = target.wstring() + L".tmp";
        std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
        if (!output) return false;

        constexpr char magic[] = "KVZHIST1";
        output.write(magic, sizeof(magic) - 1);
        const std::uint32_t count = static_cast<std::uint32_t>(
            std::min<std::size_t>(history.events().size(), 320));
        if (!writeValue(output, count)) return false;
        std::uint32_t written = 0;
        for (const auto& event : history.events()) {
            if (written++ >= count) break;
            const auto type = static_cast<std::uint32_t>(event.type);
            const std::uint32_t repeatCount = event.repeatCount;
            const std::uint8_t repeated = event.isRepeat ? 1 : 0;
            if (!writeValue(output, type) || !writeValue(output, event.wallClockMs) ||
                !writeValue(output, event.screenX) || !writeValue(output, event.screenY) ||
                !writeValue(output, event.virtualKey) || !writeValue(output, repeatCount) ||
                !writeValue(output, repeated) || !writeString(output, event.label) ||
                !writeString(output, event.chordLabel)) return false;
        }
        output.flush();
        if (!output) return false;
        output.close();
        return MoveFileExW(temporary.c_str(), target.c_str(),
            MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != FALSE;
    }

    static bool load(const std::wstring& path, HistoryStore& history) {
        if (path.empty()) return false;
        std::ifstream input(path, std::ios::binary);
        if (!input) return false;
        char magic[8]{};
        input.read(magic, sizeof(magic));
        if (!input || std::string(magic, sizeof(magic)) != "KVZHIST1")
            return false;
        std::uint32_t count = 0;
        if (!readValue(input, count) || count > 320) return false;

        std::vector<InputEvent> loaded;
        loaded.reserve(count);
        for (std::uint32_t i = 0; i < count; ++i) {
            InputEvent event;
            std::uint32_t type = 0;
            std::uint32_t repeatCount = 1;
            std::uint8_t repeated = 0;
            if (!readValue(input, type) || type > 5 ||
                !readValue(input, event.wallClockMs) ||
                !readValue(input, event.screenX) || !readValue(input, event.screenY) ||
                !readValue(input, event.virtualKey) || !readValue(input, repeatCount) ||
                !readValue(input, repeated) || repeated > 1 ||
                !readString(input, event.label) || !readString(input, event.chordLabel))
                return false;
            event.type = static_cast<InputEvent::Type>(type);
            event.repeatCount = std::max<std::uint32_t>(1, repeatCount);
            event.isRepeat = repeated != 0;
            const std::uint64_t now = wallClockNow();
            const std::uint64_t age = now >= event.wallClockMs ? now - event.wallClockMs : 0;
            const DWORD boundedAge = static_cast<DWORD>(
                std::min<std::uint64_t>(age, 0x7fffffffULL));
            event.timestamp = GetTickCount() - boundedAge;
            loaded.push_back(std::move(event));
        }
        if (input.peek() != std::char_traits<char>::eof()) return false;
        history.clear();
        for (auto it = loaded.rbegin(); it != loaded.rend(); ++it)
            history.append(std::move(*it));
        return true;
    }

private:
    template <typename T>
    static bool writeValue(std::ofstream& stream, const T& value) {
        stream.write(reinterpret_cast<const char*>(&value), sizeof(value));
        return static_cast<bool>(stream);
    }

    template <typename T>
    static bool readValue(std::ifstream& stream, T& value) {
        stream.read(reinterpret_cast<char*>(&value), sizeof(value));
        return static_cast<bool>(stream);
    }

    static bool writeString(std::ofstream& stream, const std::wstring& value) {
        if (value.size() > 4096) return false;
        const std::uint32_t length = static_cast<std::uint32_t>(value.size());
        return writeValue(stream, length) &&
            (stream.write(reinterpret_cast<const char*>(value.data()),
                static_cast<std::streamsize>(length * sizeof(wchar_t))), static_cast<bool>(stream));
    }

    static bool readString(std::ifstream& stream, std::wstring& value) {
        std::uint32_t length = 0;
        if (!readValue(stream, length) || length > 4096) return false;
        value.resize(length);
        stream.read(reinterpret_cast<char*>(value.data()),
            static_cast<std::streamsize>(length * sizeof(wchar_t)));
        return static_cast<bool>(stream);
    }

    static std::uint64_t wallClockNow() {
        return static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count());
    }
};

} // namespace keyviz
