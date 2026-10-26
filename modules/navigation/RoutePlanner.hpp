#pragma once
/**
 * @file RoutePlanner.hpp
 * @brief Chooses the ORDER in which a picker visits its shelves (and which side of each shelf).
 *
 * Problem: start at the robot, visit k shelves (any order), finish at the
 * sorter. Minimise the number of moves. This is a small Travelling Salesman
 * Problem, for which no fast exact algorithm is known.
 *
 * Three methods (compare them with: experiments routing):
 *
 *  asGiven       visit in the order the items came out of the queue. The baseline.
 *
 *  greedy        Lecture 5b greedy: from where you are, always go to the
 *                closest unvisited shelf. O(k^2) distance look-ups. Fast, but
 *                like the "krons" coin example it can miss the best answer:
 *                a choice that looks best now can force a long walk later.
 *
 *  backtracking  Lecture 5b backtracking (depth-first search tree): choose the
 *                1st shelf, then the 2nd, ... ; when all are chosen, record the
 *                route if it is the best so far; then UNDO the last choice and
 *                try the next one. This tries every order, so it is exact, but
 *                there are k! orders (x2 for the side of each shelf): only
 *                usable for small k. Two things keep it fast enough:
 *                  1. start with the greedy answer as the "best so far", and
 *                  2. prune: stop a branch as soon as
 *                     (length so far) + (straight distance from here to the end)
 *                     >= best so far. Every real route through the remaining
 *                     shelves is at least that long (triangle inequality), so
 *                     the branch cannot win.
 *
 * Distances come from the DistanceTable (cached BFS).
 */
#include <vector>

#include "core/Config.hpp"
#include "core/Types.hpp"
#include "navigation/DistanceTable.hpp"
#include "navigation/GridMap.hpp"

namespace wh {

struct RoutePlan {
    std::vector<int> order;          ///< order[i] = index into the `shelves` argument
    std::vector<CellIndex> access;   ///< where the robot stops for each visit (same order)
    int length = 0;                  ///< total moves: start -> shelves -> end
    long nodes = 0;                  ///< work done: search-tree nodes (backtracking) or options compared (greedy)
};

class RoutePlanner {
public:
    RoutePlanner(const GridMap& map, DistanceTable& distances);

    /// Uses `method`; Backtracking falls back to Greedy when there are more than `exactLimit` shelves.
    RoutePlan plan(RoutingMethod method, int exactLimit, CellIndex start, const std::vector<CellIndex>& shelves, CellIndex end);

    RoutePlan asGiven(CellIndex start, const std::vector<CellIndex>& shelves, CellIndex end);
    RoutePlan greedy(CellIndex start, const std::vector<CellIndex>& shelves, CellIndex end);
    /// `prune = false` explores the whole search tree (only for experiments: shows k! growth).
    RoutePlan backtracking(CellIndex start, const std::vector<CellIndex>& shelves, CellIndex end, bool prune = true);

private:
    /// Everything the recursive search needs, kept in one place.
    struct SearchState {
        std::vector<std::vector<CellIndex>> sides;   ///< access cells of each shelf
        CellIndex end = kNoCell;
        bool prune = true;
        std::vector<char> visited;
        std::vector<int> order;                      ///< current partial route (a stack)
        std::vector<CellIndex> access;
        RoutePlan best;
    };

    void search(SearchState& s, CellIndex position, int lengthSoFar);
    std::vector<std::vector<CellIndex>> sidesOf(const std::vector<CellIndex>& shelves) const;

    const GridMap& map_;
    DistanceTable& dist_;
};

} // namespace wh
