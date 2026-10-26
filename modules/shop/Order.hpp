#pragma once
/**
 * @file Order.hpp
 * @brief A customer order, created by Shop::checkout and sent to the warehouse.
 */
#include <string>
#include <vector>

#include "core/Types.hpp"
#include "shop/Money.hpp"

namespace wh {

enum class Priority { Standard, Express };

inline const char* toString(Priority p) { return p == Priority::Express ? "EXPRESS" : "STANDARD"; }

struct OrderLine {
    int productIndex = 0;
    std::string name;
    int qty = 0;
    Money unitPrice;
    double unitWeightKg = 0.0;
};

struct Order {
    OrderId id = kNoOrder;
    Priority priority = Priority::Standard;
    std::vector<OrderLine> lines;
    std::string placedAt;     ///< Wall-clock time, printed on the receipt.
    bool generated = false;   ///< true for automatically generated test orders.

    int units() const
    {
        int n = 0;
        for (const OrderLine& l : lines) n += l.qty;
        return n;
    }
    Money total() const
    {
        Money sum;
        for (const OrderLine& l : lines) sum += l.unitPrice * l.qty;
        return sum;
    }
    double weightKg() const
    {
        double w = 0.0;
        for (const OrderLine& l : lines) w += l.unitWeightKg * l.qty;
        return w;
    }
};

} // namespace wh
