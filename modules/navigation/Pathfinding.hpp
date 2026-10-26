#pragma once
/**
 * @file Pathfinding.hpp
 * @brief Shortest paths on the warehouse grid: Breadth-First Search and A*.
 *
 * The grid is an unweighted graph (every move costs 1), V = walkable cells,
 * E <= 4V edges.
 *
 * BFS (uses our LinkedQueue): explores cells in rings of distance 0, 1, 2 ...
 *   O(V + E). Finds shortest paths because every edge has the same cost.
 *   bfsDistances() gives the distance from ONE cell to EVERY cell, which is
 *   what route planning needs (DistanceTable caches these results).
 *
 * A* (uses our MinHeap): like BFS/Dijkstra, but always expands the cell with
 *   the smallest f = g + h, where g = moves so far and h = Manhattan distance
 *   to the goal (|dr| + |dc|). h never over-estimates the true distance on a
 *   4-connected grid (admissible) and changes by at most 1 per move
 *   (consistent), so the FIRST time the goal is taken from the heap its path
 *   is a shortest one. Worst case O(E log V), but it usually expands far fewer
 *   cells than BFS because it heads towards the goal. Used for a robot's path
 *   from where it is to ONE goal, avoiding obstacles and (if asked) other robots.
 *
 * `blocked` (optional): cells to treat as impassable in addition to walls and
 * shelves, as a vector<char> with one entry per cell (1 = blocked). An empty
 * vector means "nothing extra blocked". The start and goal cells are never
 * treated as blocked.
 */
#include <vector>

#include "core/Types.hpp"
#include "navigation/GridMap.hpp"

namespace wh {

/// Number of moves from `source` to every cell (kUnreachable if there is no path). O(V + E).
std::vector<int> bfsDistances(const GridMap& map, CellIndex source,
                              const std::vector<char>& blocked = std::vector<char>());

struct PathResult {
    bool found = false;
    std::vector<CellIndex> path;   ///< Cells to step on, WITHOUT the start, ending at the goal.
    int expanded = 0;              ///< Cells taken out of the queue/heap (a measure of work done).
};

/// Shortest path found with BFS (for comparison with A* in tests and experiments).
PathResult bfsPath(const GridMap& map, CellIndex start, CellIndex goal,
                   const std::vector<char>& blocked = std::vector<char>());

/// Shortest path found with A* and the Manhattan heuristic.
PathResult aStarPath(const GridMap& map, CellIndex start, CellIndex goal,
                     const std::vector<char>& blocked = std::vector<char>());

/// |row difference| + |column difference|: the A* heuristic.
int manhattan(const GridMap& map, CellIndex a, CellIndex b);

} // namespace wh
