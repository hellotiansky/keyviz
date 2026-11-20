#pragma once

#include "keyviz/filter_engine.h"
#include "keyviz/history_store.h"

namespace keyviz {

class EventEngine {
public:
    void process(InputEvent event);
    const HistoryStore& history() const noexcept { return history_; }

private:
    FilterEngine filter_;
    HistoryStore history_;
};

} // namespace keyviz
