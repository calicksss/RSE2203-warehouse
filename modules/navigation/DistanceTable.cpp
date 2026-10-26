#include "navigation/DistanceTable.hpp"

#include "navigation/Pathfinding.hpp"

namespace wh {

DistanceTable::DistanceTable(const GridMap& map) : map_(map), fromCell_(static_cast<std::size_t>(map.cellCount())) {}

int DistanceTable::distance(CellIndex from, CellIndex to)
{
    // Only walkable cells are graph vertices. (Without this check, a BFS started on a
    // shelf would give answers that break the symmetry used below.)
    if (!map_.isWalkable(from) || !map_.isWalkable(to)) return kUnreachable;

    // Moves are reversible (no one-way aisles), so distance(a, b) == distance(b, a).
    // If we already have a table for `to`, use it instead of running a new BFS.
    std::vector<int>& reverse = fromCell_[static_cast<std::size_t>(to)];
    if (!reverse.empty()) return reverse[static_cast<std::size_t>(from)];

    std::vector<int>& table = fromCell_[static_cast<std::size_t>(from)];
    if (table.empty()) {
        table = bfsDistances(map_, from);   // cache miss: one BFS
        ++bfsRuns_;
    }
    return table[static_cast<std::size_t>(to)];
}

} // namespace wh
