#include "navigation/Pathfinding.hpp"

#include <algorithm>
#include <cstdlib>

#include "ds/LinkedQueue.hpp"
#include "ds/MinHeap.hpp"

namespace wh {

namespace {

bool isBlocked(const std::vector<char>& blocked, CellIndex c)
{
    return !blocked.empty() && blocked[static_cast<std::size_t>(c)] != 0;
}

/// Walks the parent links back from the goal and returns start->goal (without the start).
std::vector<CellIndex> rebuildPath(const std::vector<CellIndex>& parent, CellIndex start, CellIndex goal)
{
    std::vector<CellIndex> path;
    for (CellIndex c = goal; c != start; c = parent[static_cast<std::size_t>(c)]) path.push_back(c);
    std::reverse(path.begin(), path.end());
    return path;
}

} // namespace

int manhattan(const GridMap& map, CellIndex a, CellIndex b)
{
    const GridPos p = map.pos(a);
    const GridPos q = map.pos(b);
    return std::abs(p.row - q.row) + std::abs(p.col - q.col);
}

std::vector<int> bfsDistances(const GridMap& map, CellIndex source, const std::vector<char>& blocked)
{
    std::vector<int> dist(static_cast<std::size_t>(map.cellCount()), kUnreachable);
    LinkedQueue<CellIndex> frontier;
    dist[static_cast<std::size_t>(source)] = 0;
    frontier.enqueue(source);

    CellIndex nb[4];
    while (!frontier.empty()) {
        const CellIndex c = frontier.dequeue();
        const int n = map.neighbours(c, nb);
        for (int i = 0; i < n; ++i) {
            const CellIndex next = nb[i];
            if (dist[static_cast<std::size_t>(next)] != kUnreachable) continue;   // already discovered
            if (isBlocked(blocked, next)) continue;
            dist[static_cast<std::size_t>(next)] = dist[static_cast<std::size_t>(c)] + 1;
            frontier.enqueue(next);
        }
    }
    return dist;
}

PathResult bfsPath(const GridMap& map, CellIndex start, CellIndex goal, const std::vector<char>& blocked)
{
    PathResult result;
    std::vector<CellIndex> parent(static_cast<std::size_t>(map.cellCount()), kNoCell);
    std::vector<char> seen(static_cast<std::size_t>(map.cellCount()), 0);
    LinkedQueue<CellIndex> frontier;
    seen[static_cast<std::size_t>(start)] = 1;
    frontier.enqueue(start);

    CellIndex nb[4];
    while (!frontier.empty()) {
        const CellIndex c = frontier.dequeue();
        ++result.expanded;
        if (c == goal) {
            result.found = true;
            result.path = rebuildPath(parent, start, goal);
            return result;
        }
        const int n = map.neighbours(c, nb);
        for (int i = 0; i < n; ++i) {
            const CellIndex next = nb[i];
            if (seen[static_cast<std::size_t>(next)]) continue;
            if (next != goal && isBlocked(blocked, next)) continue;
            seen[static_cast<std::size_t>(next)] = 1;
            parent[static_cast<std::size_t>(next)] = c;
            frontier.enqueue(next);
        }
    }
    return result;   // goal not reachable
}

namespace {

/// An entry in the A* open list. The heap gives us the smallest f first;
/// on equal f we prefer the smaller h (the cell closer to the goal), which
/// tends to expand fewer cells.
struct OpenEntry {
    int f;
    int h;
    CellIndex cell;
    bool operator<(const OpenEntry& o) const { return f != o.f ? f < o.f : h < o.h; }
};

} // namespace

PathResult aStarPath(const GridMap& map, CellIndex start, CellIndex goal, const std::vector<char>& blocked)
{
    PathResult result;
    const std::size_t cells = static_cast<std::size_t>(map.cellCount());
    std::vector<int> g(cells, kUnreachable);        // best known moves from start
    std::vector<CellIndex> parent(cells, kNoCell);
    std::vector<char> closed(cells, 0);             // 1 = shortest path to this cell is final
    MinHeap<OpenEntry> open;

    g[static_cast<std::size_t>(start)] = 0;
    const int h0 = manhattan(map, start, goal);
    open.push({h0, h0, start});

    CellIndex nb[4];
    while (!open.empty()) {
        const OpenEntry current = open.pop();
        const std::size_t cu = static_cast<std::size_t>(current.cell);
        // The same cell can be in the heap several times (we push again when we
        // find a shorter way instead of updating the old entry). Only the first,
        // smallest one counts; skip the stale copies. ("lazy deletion")
        if (closed[cu]) continue;
        closed[cu] = 1;
        ++result.expanded;

        if (current.cell == goal) {
            result.found = true;
            result.path = rebuildPath(parent, start, goal);
            return result;
        }

        const int n = map.neighbours(current.cell, nb);
        for (int i = 0; i < n; ++i) {
            const CellIndex next = nb[i];
            const std::size_t nu = static_cast<std::size_t>(next);
            if (closed[nu]) continue;
            if (next != goal && isBlocked(blocked, next)) continue;
            const int tentative = g[cu] + 1;
            if (tentative < g[nu]) {                // found a shorter way to `next`
                g[nu] = tentative;
                parent[nu] = current.cell;
                const int h = manhattan(map, next, goal);
                open.push({tentative + h, h, next});
            }
        }
    }
    return result;   // open list empty: goal not reachable
}

} // namespace wh
