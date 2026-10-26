#pragma once
/**
 * @file EventLog.hpp
 * @brief Rolling list of the most recent events ("Order #3 packed", ...), shown by the UI.
 *
 * Robots and the job manager only ADD events; the UI decides how to show them.
 * That keeps the model free of any printing.
 *
 * Data structure: std::deque, because we add at the back and drop the oldest
 * at the front, both O(1). (A std::vector would need erase(begin()), O(n).)
 */
#include <cstddef>
#include <deque>
#include <string>

#include "core/Types.hpp"

namespace wh {

enum class Severity { Info, Success, Warning, Alert };

struct Event {
    Tick tick = 0;
    Severity severity = Severity::Info;
    std::string source;   ///< Who reported it, e.g. "PCK-3" or "JOBS".
    std::string text;
};

class EventLog {
public:
    explicit EventLog(std::size_t capacity = 400) : capacity_(capacity) {}

    void add(Tick tick, Severity severity, const std::string& source, const std::string& text)
    {
        events_.push_back({tick, severity, source, text});
        if (events_.size() > capacity_) events_.pop_front();
        ++total_;
    }

    const std::deque<Event>& events() const { return events_; }
    std::size_t total() const { return total_; }

private:
    std::size_t capacity_;
    std::size_t total_ = 0;
    std::deque<Event> events_;
};

} // namespace wh
