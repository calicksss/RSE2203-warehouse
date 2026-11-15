#pragma once
/**
 * @file Battery.hpp
 * @brief A robot battery level in percent, always kept between 0 and 100.
 */
#include <algorithm>

namespace wh {

class Battery {
public:
    explicit Battery(double percent = 100.0) : level_(clamp(percent)) {}

    double percent() const { return level_; }
    bool isEmpty() const { return level_ <= 0.0; }

    // Guard against negative amounts so drain/charge cannot inadvertently invert behavior
    void drain(double amount) { if (amount > 0.0) level_ = clamp(level_ - amount); }
    void charge(double amount) { if (amount > 0.0) level_ = clamp(level_ + amount); }

private:
    static double clamp(double v) { return std::min(100.0, std::max(0.0, v)); }
    double level_;
};

} // namespace wh
