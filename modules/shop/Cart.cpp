#include "shop/Cart.hpp"

#include <cstdio>

namespace wh {

namespace {
std::string kg(double v)
{
    char buf[32];
    std::snprintf(buf, sizeof buf, "%.2f kg", v);
    return buf;
}
} // namespace

Cart::Result Cart::add(const Catalogue& catalogue, int productIndex, int qty)
{
    if (productIndex < 0 || productIndex >= catalogue.size()) return {false, "Unknown product."};
    if (qty <= 0) return {false, "Quantity must be a positive whole number."};

    const Product& p = catalogue.at(productIndex);
    const int already = quantityOf(productIndex);
    if (already + qty > p.stock)
        return {false, "Only " + std::to_string(p.stock - already) + " more " + p.name + " available."};

    const double newWeight = weightKg(catalogue) + p.weightKg * qty;
    if (newWeight > weightLimitKg_ + 1e-9)   // small tolerance for floating-point rounding
        return {false, "Too heavy: the order would weigh " + kg(newWeight) + " (limit " + kg(weightLimitKg_) + ")."};

    lines_[productIndex] += qty;
    return {true, "Added " + std::to_string(qty) + " x " + p.name + " to cart."};
}

Cart::Result Cart::remove(const Catalogue& catalogue, int productIndex, int qty)
{
    const auto it = lines_.find(productIndex);
    if (it == lines_.end()) return {false, "That product is not in your cart."};
    if (qty <= 0) return {false, "Quantity must be a positive whole number."};

    const std::string& name = catalogue.at(productIndex).name;
    if (qty >= it->second) {
        lines_.erase(it);
        return {true, "Removed all " + name + " from cart."};
    }
    it->second -= qty;
    return {true, "Removed " + std::to_string(qty) + " x " + name + " from cart."};
}

int Cart::quantityOf(int productIndex) const
{
    const auto it = lines_.find(productIndex);
    return it == lines_.end() ? 0 : it->second;
}

int Cart::units() const
{
    int n = 0;
    for (const auto& line : lines_) n += line.second;
    return n;
}

Money Cart::total(const Catalogue& catalogue) const
{
    Money sum;
    for (const auto& line : lines_) sum += catalogue.at(line.first).price * line.second;
    return sum;
}

double Cart::weightKg(const Catalogue& catalogue) const
{
    double w = 0.0;
    for (const auto& line : lines_) w += catalogue.at(line.first).weightKg * line.second;
    return w;
}

} // namespace wh
