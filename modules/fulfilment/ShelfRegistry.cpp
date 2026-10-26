#include "fulfilment/ShelfRegistry.hpp"

#include <string>

#include "core/Errors.hpp"

namespace wh {

ShelfRegistry::ShelfRegistry(const GridMap& map, const Catalogue& catalogue)
    : productAtCell_(static_cast<std::size_t>(map.cellCount()), -1)
{
    for (int i = 0; i < catalogue.size(); ++i) {
        const Product& p = catalogue.at(i);
        if (!map.inBounds(p.shelf) || map.tile(map.index(p.shelf)) != Tile::Shelf)
            throw DataError("products", i + 1, "'" + p.name + "' is on " + toString(p.shelf) + ", which is not a shelf 'S' in the map");
        const CellIndex cell = map.index(p.shelf);
        shelfOfProduct_.push_back(cell);
        productAtCell_[static_cast<std::size_t>(cell)] = i;
    }
    if (catalogue.size() != static_cast<int>(map.shelves().size())) {
        std::string empty;
        for (CellIndex s : map.shelves())
            if (productAtCell_[static_cast<std::size_t>(s)] < 0 && empty.size() < 80) empty += " " + toString(map.pos(s));
        throw DataError("products", 0,
                        "there are " + std::to_string(catalogue.size()) + " products but " + std::to_string(map.shelves().size()) +
                            " shelves; every shelf needs exactly one product." + (empty.empty() ? "" : " Empty shelves:" + empty));
    }
}

} // namespace wh
