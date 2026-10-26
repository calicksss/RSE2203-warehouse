#pragma once
/**
 * @file Cart.hpp
 * @brief The customer's shopping cart, with input checks.
 *
 * Rules (each one has a unit test):
 *  - quantity must be positive,
 *  - you cannot put more in the cart than the shelf has in stock,
 *  - the whole order must weigh at most the weight limit (a picker's payload).
 * Stock is only CHECKED here. It is taken away at checkout, so leaving with a
 * full cart never changes the stock.
 *
 * Data structure: std::map<productIndex, qty> (a balanced binary search tree).
 * Lookup/insert O(log n), and lines come out sorted by product id for display.
 */
#include <map>
#include <string>

#include "shop/Catalogue.hpp"

namespace wh {

class Cart {
public:
    struct Result {
        bool ok = false;
        std::string message;   ///< Shown to the user either way.
    };

    explicit Cart(double weightLimitKg = 25.0) : weightLimitKg_(weightLimitKg) {}

    Result add(const Catalogue& catalogue, int productIndex, int qty);
    Result remove(const Catalogue& catalogue, int productIndex, int qty);
    void clear() { lines_.clear(); }

    bool empty() const { return lines_.empty(); }
    int quantityOf(int productIndex) const;
    int units() const;
    Money total(const Catalogue& catalogue) const;
    double weightKg(const Catalogue& catalogue) const;
    double weightLimitKg() const { return weightLimitKg_; }

    const std::map<int, int>& lines() const { return lines_; }

private:
    std::map<int, int> lines_;
    double weightLimitKg_;
};

} // namespace wh
