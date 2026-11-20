#pragma once

#include "keyviz/input_event.h"

namespace keyviz {

class FilterEngine {
public:
    // Placeholder policy: reject unnamed/invalid events. User-configurable
    // include/exclude rules will be loaded from Settings in a later milestone.
    bool allow(const InputEvent& event) const noexcept {
        return !event.label.empty();
    }
};

} // namespace keyviz
