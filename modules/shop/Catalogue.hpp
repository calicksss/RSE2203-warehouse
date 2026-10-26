#pragma once
/**
 * @file Catalogue.hpp
 * @brief The products for sale, loaded from data/Products.txt.
 *
 * File format, one product per line ('#' starts a comment):
 *     Name  Price  WeightKg  Stock  ShelfRow  ShelfCol  [Category]
 * Underscores in Name and Category are shown as spaces.
 *
 * A product's index in the file (0, 1, 2 ...) is its id everywhere in the program.
 *
 * Searching by name (Lecture 5a, binary search):
 *   After loading, we build `byName_`: the product indices sorted by lower-case
 *   name with mergeSort (O(n log n), done once). findByName() then uses binary
 *   search: O(log n) per lookup instead of O(n) for a linear scan.
 *   The search box in the UI matches PARTS of names ("ric" -> "Bag of rice"),
 *   which binary search cannot do, so search() is a linear scan: O(n * m).
 */
#include <iosfwd>
#include <string>
#include <vector>

#include "core/Types.hpp"
#include "shop/Money.hpp"

namespace wh {

struct Product {
    std::string name;
    std::string category;
    Money price;
    double weightKg = 0.0;
    int stock = 0;
    GridPos shelf;   ///< Map coordinate of the shelf that holds this product.
};

class Catalogue {
public:
    /// Reads products from a stream. Throws DataError (with the line number) on bad input.
    static Catalogue parse(std::istream& in, const std::string& sourceName = "products");
    static Catalogue loadFromFile(const std::string& path);

    int size() const { return static_cast<int>(products_.size()); }
    const Product& at(int index) const { return products_.at(static_cast<std::size_t>(index)); }
    const std::vector<Product>& all() const { return products_; }

    /// Exact, case-insensitive name lookup by binary search. Returns the product index or -1.
    int findByName(const std::string& name) const;

    /// Indices of products whose name or category contains `text` (case-insensitive), in id order.
    std::vector<int> search(const std::string& text) const;

    /// Categories in the order they first appear in the file.
    const std::vector<std::string>& categories() const { return categories_; }

    /// Removes `qty` units from stock. Returns false (and changes nothing) if there are not enough.
    bool consumeStock(int index, int qty);

private:
    std::vector<Product> products_;
    std::vector<std::string> categories_;
    std::vector<int> byName_;   ///< Product indices sorted by lower-case name (for binary search).
};

std::string toLower(const std::string& text);

} // namespace wh
