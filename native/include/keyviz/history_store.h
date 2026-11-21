#pragma once
#include "keyviz/input_event.h"
#include <cstddef>
#include <deque>
#include <utility>
namespace keyviz {
class HistoryStore {
public:
    explicit HistoryStore(std::size_t capacity = 80) : capacity_(capacity) {}
    void setCapacity(std::size_t capacity) {
        capacity_ = capacity;
        while (events_.size() > capacity_) events_.pop_back();
    }
    std::size_t capacity() const noexcept { return capacity_; }
    void append(InputEvent event) {
        if (capacity_ == 0) return;
        if (event.type == InputEvent::Type::MouseMove && !events_.empty() && events_.front().type == InputEvent::Type::MouseMove) {
            events_.front() = std::move(event); return;
        }
        if (event.type == InputEvent::Type::KeyDown && event.isRepeat && !events_.empty() &&
            events_.front().type == InputEvent::Type::KeyDown && events_.front().virtualKey == event.virtualKey) {
            ++events_.front().repeatCount; events_.front().timestamp = event.timestamp; events_.front().isRepeat = true; return;
        }
        events_.push_front(std::move(event));
        if (events_.size() > capacity_) events_.pop_back();
    }
    void replaceEvents(std::deque<InputEvent> events) {
        events_ = std::move(events);
        while (events_.size() > capacity_) events_.pop_back();
    }
    const std::deque<InputEvent>& events() const noexcept { return events_; }
    void clear() noexcept { events_.clear(); }
private:
    std::size_t capacity_;
    std::deque<InputEvent> events_;
};
} // namespace keyviz
