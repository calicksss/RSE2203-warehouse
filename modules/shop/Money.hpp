#pragma once
/**
 * @file Money.hpp
 * @brief An exact amount of money stored as a whole number of cents.
 *
 * Why not double? Binary floating point cannot store most decimal prices
 * exactly: 0.10 + 0.20 gives 0.30000000000000004. Adding hundreds of prices as
 * doubles slowly drifts away from the right total. Integers are exact, so we
 * keep cents in a long long and only format as "$12.50" for display.
 */
#include <string>

namespace wh {

class Money {
public:
    Money() = default;
    static Money fromCents(long long cents) { Money m; m.cents_ = cents; return m; }

    /// Parses "12", "12.5" or "12.50" (at most two decimals). Returns false on bad input.
    static bool parse(const std::string& text, Money& out);

    long long cents() const { return cents_; }
    std::string str() const;   ///< "$12.50"

    Money& operator+=(Money o) { cents_ += o.cents_; return *this; }
    Money operator+(Money o) const { return fromCents(cents_ + o.cents_); }
    Money operator*(int n) const { return fromCents(cents_ * n); }
    bool operator==(Money o) const { return cents_ == o.cents_; }
    bool operator!=(Money o) const { return cents_ != o.cents_; }
    bool operator<(Money o) const { return cents_ < o.cents_; }

private:
    long long cents_ = 0;
};

} // namespace wh
