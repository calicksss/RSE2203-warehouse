#pragma once
/**
 * @file DistanceTable.hpp
 * @brief Walking distance between any two cells of the (empty) warehouse, with caching.
 *
 * The route planner asks "how far is shelf A from shelf B?" thousands of
 * times. Running a BFS for every question would repeat the same work, so the
 * first time we need distances FROM a cell we run one BFS (O(V + E)) and keep
 * the whole result. Every later question from that cell is an O(1) array
 * lookup. Remembering results of earlier calls is called memoisation (compare
 * the slow recursive Fibonacci in Lecture 5a, which recomputes the same values).
 *
 * The distances ignore temporary obstacles and other robots: they are the
 * "normal" distances used for PLANNING (route order, which robot is nearest).
 * Actual driving uses A* with the obstacles of the moment.
 *
 * Memory: at most one int per cell per source cell (693 x 693 ints ~ 2 MB).
 */
#include <vector>

#include "core/Types.hpp"
#include "navigation/GridMap.hpp"

namespace wh {

class DistanceTable {
public:
    explicit DistanceTable(const GridMap& map);

    /// Moves needed to go from `from` to `to`. Both must be walkable cells (floor or dock);
    /// otherwise, or if there is no path, the answer is kUnreachable.
    int distance(CellIndex from, CellIndex to);

    /// Number of BFS runs so far (each one is a cache miss).
    int bfsRuns() const { return bfsRuns_; }

private:
    const GridMap& map_;
    std::vector<std::vector<int>> fromCell_;   ///< fromCell_[c] empty = not computed yet
    int bfsRuns_ = 0;
};

} // namespace wh
