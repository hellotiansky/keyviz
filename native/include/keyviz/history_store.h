#pragma once

#include "keyviz/input_event.h"

#include <cstddef>
#include <deque>

namespace keyviz {

class HistoryStore {
public:
    explicit HistoryStore(std::size_t capacity = 80) : capacity_(capacity) {}

    void append(InputEvent event) {
        if (capacity_ == 0) return;
        events_.push_front(std::move(event));
        if (events_.size() > capacity_) events_.pop_back();
    }

    const std::deque<InputEvent>& events() const noexcept { return events_; }
    void clear() noexcept { events_.clear(); }

private:
    std::size_t capacity_;
    std::deque<InputEvent> events_;
};

} // namespace keyviz
