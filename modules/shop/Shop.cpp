#include "shop/Shop.hpp"

#include <algorithm>
#include <exception>

#include "shop/Receipt.hpp"

namespace wh {

Shop::Shop(const Catalogue& catalogue, const std::string& receiptDirectory, double orderWeightLimitKg)
    : catalogue_(catalogue), cart_(orderWeightLimitKg), receiptDir_(receiptDirectory)
{}

Shop::CheckoutResult Shop::checkout(Priority priority)
{
    CheckoutResult result;
    if (cart_.empty()) {
        result.message = "Your cart is empty.";
        return result;
    }
    // 1) Check every line BEFORE changing anything (all-or-nothing).
    for (const auto& line : cart_.lines()) {
        if (catalogue_.at(line.first).stock < line.second) {
            result.message = catalogue_.at(line.first).name + " no longer has enough stock.";
            return result;
        }
    }
    // 2) Build the order and take the stock.
    Order order;
    order.id = nextOrderId_++;
    order.priority = priority;
    order.placedAt = currentTimestamp();
    for (const auto& line : cart_.lines()) {
        const Product& p = catalogue_.at(line.first);
        order.lines.push_back({line.first, p.name, line.second, p.price, p.weightKg});
        catalogue_.consumeStock(line.first, line.second);
    }
    cart_.clear();

    // 3) Receipt. A failed file write must not undo a paid order, so we only report it.
    result.ok = true;
    result.message = "Order " + Receipt::reference(order) + " placed.";
    if (!receiptDir_.empty()) {
        try {
            result.receiptPath = Receipt::save(order, receiptDir_);
            result.message += " Receipt saved.";
        } catch (const std::exception& e) {
            result.message += std::string(" (Receipt not saved: ") + e.what() + ")";
        }
    }
    result.order = order;
    history_.push_back(order);
    return result;
}

std::vector<Order> Shop::generateOrders(int count, std::mt19937& rng, double expressShare)
{
    std::vector<Order> created;
    std::uniform_int_distribution<int> linesDist(1, 4);
    std::uniform_int_distribution<int> qtyDist(1, 3);
    std::bernoulli_distribution expressDist(expressShare);

    std::vector<int> indices;
    for (int i = 0; i < catalogue_.size(); ++i) indices.push_back(i);

    for (int n = 0; n < count; ++n) {
        std::shuffle(indices.begin(), indices.end(), rng);   // pick random distinct products
        Order order;
        order.priority = expressDist(rng) ? Priority::Express : Priority::Standard;
        order.generated = true;
        const int wantLines = linesDist(rng);
        double weight = 0.0;
        for (int idx : indices) {
            if (static_cast<int>(order.lines.size()) == wantLines) break;
            const Product& p = catalogue_.at(idx);
            const int qty = std::min(qtyDist(rng), p.stock);
            if (qty <= 0 || weight + p.weightKg * qty > cart_.weightLimitKg()) continue;
            order.lines.push_back({idx, p.name, qty, p.price, p.weightKg});
            weight += p.weightKg * qty;
        }
        if (order.lines.empty()) break;   // everything is sold out
        for (const OrderLine& l : order.lines) catalogue_.consumeStock(l.productIndex, l.qty);
        order.id = nextOrderId_++;
        order.placedAt = currentTimestamp();
        history_.push_back(order);
        created.push_back(order);
    }
    return created;
}

} // namespace wh
