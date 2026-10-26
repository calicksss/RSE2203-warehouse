#pragma once
/**
 * @file Shop.hpp
 * @brief The storefront: catalogue + cart + checkout. Turns a cart into an Order.
 *
 * Shop knows nothing about robots or the screen. The UI calls it, and the
 * Orders it creates are handed to the warehouse Simulation.
 */
#include <random>
#include <string>
#include <vector>

#include "shop/Cart.hpp"
#include "shop/Catalogue.hpp"
#include "shop/Order.hpp"

namespace wh {

class Shop {
public:
    /// receiptDirectory "" = do not write receipt files (used by tests and experiments).
    Shop(const Catalogue& catalogue, const std::string& receiptDirectory, double orderWeightLimitKg = 25.0);

    const Catalogue& catalogue() const { return catalogue_; }
    Cart& cart() { return cart_; }
    const Cart& cart() const { return cart_; }

    struct CheckoutResult {
        bool ok = false;
        std::string message;
        Order order;               ///< Valid only when ok is true.
        std::string receiptPath;   ///< "" if no receipt was written.
    };

    /// All-or-nothing: either every line is in stock and the order is created
    /// (stock taken, receipt written, cart emptied), or nothing changes.
    CheckoutResult checkout(Priority priority);

    /// Creates `count` random customer orders (for demos and experiments). Takes stock; no receipts.
    std::vector<Order> generateOrders(int count, std::mt19937& rng, double expressShare = 0.2);

    const std::vector<Order>& history() const { return history_; }

private:
    Catalogue catalogue_;   ///< Our own copy: stock changes as orders are placed.
    Cart cart_;
    std::vector<Order> history_;
    std::string receiptDir_;
    OrderId nextOrderId_ = 1;
};

} // namespace wh
