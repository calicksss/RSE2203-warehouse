#pragma once
/**
 * @file ShelfRegistry.hpp
 * @brief Which shelf cell holds which product, checked against the map when the program starts.
 *
 * Both directions are plain array look-ups, O(1):
 *   shelfOf(product)  -> vector indexed by product index
 *   productAt(cell)   -> vector indexed by cell (-1 = no product)
 *
 * The constructor throws DataError if the data files disagree: a product on a
 * cell that is not 'S', or a different number of products and shelves.
 */
#include <vector>

#include "core/Types.hpp"
#include "navigation/GridMap.hpp"
#include "shop/Catalogue.hpp"

namespace wh {

class ShelfRegistry {
public:
    ShelfRegistry(const GridMap& map, const Catalogue& catalogue);

    CellIndex shelfOf(int productIndex) const { return shelfOfProduct_.at(static_cast<std::size_t>(productIndex)); }
    int productAt(CellIndex cell) const { return productAtCell_.at(static_cast<std::size_t>(cell)); }

private:
    std::vector<CellIndex> shelfOfProduct_;
    std::vector<int> productAtCell_;
};

} // namespace wh
