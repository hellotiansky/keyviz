#pragma once

#include "keyviz/filter_engine.h"
#include "keyviz/history_store.h"

namespace keyviz {

class EventEngine {
public:
    void process(InputEvent event);
    void setFilterSettings(const FilterSettings& settings) noexcept {
        filter_.setSettings(settings);
    }
    const FilterSettings& filterSettings() const noexcept {
        return filter_.settings();
    }
    void setHistoryCapacity(std::size_t capacity) {
        history_.setCapacity(capacity);
    }
    const HistoryStore& history() const noexcept { return history_; }
    void clearHistory() noexcept { history_.clear(); }

private:
    FilterEngine filter_;
    HistoryStore history_;
};

} // namespace keyviz
