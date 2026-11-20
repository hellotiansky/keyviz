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
    const HistoryStore& history() const noexcept { return history_; }

private:
    FilterEngine filter_;
    HistoryStore history_;
};

} // namespace keyviz
