#include "shop/Money.hpp"

#include <cctype>

namespace wh {

bool Money::parse(const std::string& text, Money& out)
{
    std::size_t i = 0;
    long long whole = 0;
    int wholeDigits = 0;
    while (i < text.size() && std::isdigit(static_cast<unsigned char>(text[i]))) {
        whole = whole * 10 + (text[i] - '0');
        if (++wholeDigits > 9) return false;   // refuse absurd amounts instead of overflowing
        ++i;
    }

    long long fraction = 0;
    int decimals = 0;
    if (i < text.size()) {
        if (text[i] != '.') return false;
        ++i;
        while (i < text.size()) {
            if (!std::isdigit(static_cast<unsigned char>(text[i])) || decimals == 2) return false;
            fraction = fraction * 10 + (text[i] - '0');
            ++decimals;
            ++i;
        }
        if (decimals == 1) fraction *= 10;     // "12.5" means 50 cents
    }
    if (wholeDigits == 0 && decimals == 0) return false;   // "", "." or "-1" are not prices

    out = fromCents(whole * 100 + fraction);
    return true;
}

std::string Money::str() const
{
    const long long absCents = cents_ < 0 ? -cents_ : cents_;
    std::string cents = std::to_string(absCents % 100);
    if (cents.size() < 2) cents = "0" + cents;
    return std::string(cents_ < 0 ? "-$" : "$") + std::to_string(absCents / 100) + "." + cents;
}

} // namespace wh
