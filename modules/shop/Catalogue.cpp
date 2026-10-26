#include "shop/Catalogue.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

#include "core/Errors.hpp"
#include "ds/Search.hpp"
#include "ds/Sorting.hpp"

namespace wh {

std::string toLower(const std::string& text)
{
    std::string out = text;
    for (char& c : out) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return out;
}

namespace {

std::string underscoresToSpaces(std::string s)
{
    std::replace(s.begin(), s.end(), '_', ' ');
    return s;
}

/// Reads a whole token as a number; "2.5kg" or "" fail.
template <typename T>
bool parseNumber(const std::string& token, T& out)
{
    std::istringstream iss(token);
    iss >> out;
    return !iss.fail() && iss.eof();
}

} // namespace

Catalogue Catalogue::parse(std::istream& in, const std::string& sourceName)
{
    Catalogue cat;
    std::string line;
    int lineNo = 0;

    while (std::getline(in, line)) {
        ++lineNo;
        const std::size_t hash = line.find('#');
        if (hash != std::string::npos) line.erase(hash);   // strip comments
        std::istringstream iss(line);
        std::vector<std::string> tok;
        std::string t;
        while (iss >> t) tok.push_back(t);
        if (tok.empty()) continue;                         // blank line
        if (tok.size() != 6 && tok.size() != 7)
            throw DataError(sourceName, lineNo,
                            "expected 'Name Price Weight Stock Row Col [Category]', got " + std::to_string(tok.size()) + " fields");

        Product p;
        p.name = underscoresToSpaces(tok[0]);
        if (!Money::parse(tok[1], p.price) || p.price.cents() <= 0)
            throw DataError(sourceName, lineNo, "invalid price '" + tok[1] + "'");
        if (!parseNumber(tok[2], p.weightKg) || p.weightKg <= 0.0)
            throw DataError(sourceName, lineNo, "invalid weight '" + tok[2] + "'");
        if (!parseNumber(tok[3], p.stock) || p.stock < 0)
            throw DataError(sourceName, lineNo, "invalid stock '" + tok[3] + "'");
        if (!parseNumber(tok[4], p.shelf.row) || !parseNumber(tok[5], p.shelf.col))
            throw DataError(sourceName, lineNo, "invalid shelf coordinate");
        p.category = tok.size() == 7 ? underscoresToSpaces(tok[6]) : "General";

        // Duplicate checks are a linear scan: O(n) per product, O(n^2) per file.
        // Fine for ~100 products and only done once at start-up.
        for (const Product& other : cat.products_) {
            if (toLower(other.name) == toLower(p.name))
                throw DataError(sourceName, lineNo, "duplicate product name '" + p.name + "'");
            if (other.shelf == p.shelf)
                throw DataError(sourceName, lineNo, "shelf " + toString(p.shelf) + " already holds '" + other.name + "'");
        }
        if (std::find(cat.categories_.begin(), cat.categories_.end(), p.category) == cat.categories_.end())
            cat.categories_.push_back(p.category);
        cat.products_.push_back(p);
    }
    if (cat.products_.empty()) throw DataError(sourceName, 0, "no products found");

    // Build the name index once: O(n log n).
    for (int i = 0; i < cat.size(); ++i) cat.byName_.push_back(i);
    const std::vector<Product>& products = cat.products_;
    mergeSort(cat.byName_, [&products](int a, int b) {
        return toLower(products[static_cast<std::size_t>(a)].name) < toLower(products[static_cast<std::size_t>(b)].name);
    });
    return cat;
}

Catalogue Catalogue::loadFromFile(const std::string& path)
{
    std::ifstream file(path);
    if (!file) throw DataError(path, 0, "cannot open product file");
    return parse(file, path);
}

int Catalogue::findByName(const std::string& name) const
{
    const std::string key = toLower(name);
    // compare(element, key): element is a product index in byName_.
    const int pos = binarySearch(byName_, key, [this](int productIndex, const std::string& k) {
        return toLower(at(productIndex).name).compare(k);
    });
    return pos < 0 ? -1 : byName_[static_cast<std::size_t>(pos)];
}

std::vector<int> Catalogue::search(const std::string& text) const
{
    const std::string needle = toLower(text);
    std::vector<int> found;
    for (int i = 0; i < size(); ++i) {
        const Product& p = at(i);
        if (toLower(p.name).find(needle) != std::string::npos || toLower(p.category).find(needle) != std::string::npos)
            found.push_back(i);
    }
    return found;
}

bool Catalogue::consumeStock(int index, int qty)
{
    Product& p = products_.at(static_cast<std::size_t>(index));
    if (qty <= 0 || p.stock < qty) return false;
    p.stock -= qty;
    return true;
}

} // namespace wh
