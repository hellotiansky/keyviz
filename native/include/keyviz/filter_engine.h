#pragma once

#include "keyviz/input_event.h"
#include "keyviz/settings_store.h"

namespace keyviz {

class FilterEngine {
public:
    explicit FilterEngine(FilterSettings settings = {}) : settings_(settings) {}

    void setSettings(const FilterSettings& settings) noexcept {
        settings_ = settings;
    }

    const FilterSettings& settings() const noexcept { return settings_; }

    bool allow(const InputEvent& event) const noexcept {
        if (event.label.empty()) return false;
        switch (event.type) {
        case InputEvent::Type::KeyDown:
        case InputEvent::Type::KeyUp:
            return settings_.keyboard;
        case InputEvent::Type::MouseDown:
        case InputEvent::Type::MouseUp:
            return settings_.mouseButtons;
        case InputEvent::Type::Wheel:
            return settings_.mouseWheel;
        case InputEvent::Type::MouseMove:
            return settings_.mouseMove;
        }
        return false;
    }

private:
    FilterSettings settings_;
};

} // namespace keyviz
