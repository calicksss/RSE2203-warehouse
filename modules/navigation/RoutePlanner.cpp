#include "navigation/RoutePlanner.hpp"

namespace wh {

RoutePlanner::RoutePlanner(const GridMap& map, DistanceTable& distances) : map_(map), dist_(distances) {}

std::vector<std::vector<CellIndex>> RoutePlanner::sidesOf(const std::vector<CellIndex>& shelves) const
{
    std::vector<std::vector<CellIndex>> sides;
    for (CellIndex shelf : shelves) sides.push_back(map_.accessCells(shelf));
    return sides;
}

RoutePlan RoutePlanner::plan(RoutingMethod method, int exactLimit, CellIndex start, const std::vector<CellIndex>& shelves,
                             CellIndex end)
{
    switch (method) {
        case RoutingMethod::AsGiven: return asGiven(start, shelves, end);
        case RoutingMethod::Greedy:  return greedy(start, shelves, end);
        case RoutingMethod::Backtracking:
            if (static_cast<int>(shelves.size()) <= exactLimit) return backtracking(start, shelves, end);
            return greedy(start, shelves, end);   // too many shelves for an exact search
    }
    return greedy(start, shelves, end);
}

RoutePlan RoutePlanner::asGiven(CellIndex start, const std::vector<CellIndex>& shelves, CellIndex end)
{
    RoutePlan plan;
    const std::vector<std::vector<CellIndex>> sides = sidesOf(shelves);
    CellIndex position = start;
    for (std::size_t i = 0; i < shelves.size(); ++i) {
        // Fixed order; only the side of the shelf is chosen (the nearer one).
        CellIndex bestSide = sides[i][0];
        for (CellIndex a : sides[i])
            if (dist_.distance(position, a) < dist_.distance(position, bestSide)) bestSide = a;
        plan.order.push_back(static_cast<int>(i));
        plan.access.push_back(bestSide);
        plan.length += dist_.distance(position, bestSide);
        position = bestSide;
        ++plan.nodes;
    }
    plan.length += dist_.distance(position, end);
    return plan;
}

RoutePlan RoutePlanner::greedy(CellIndex start, const std::vector<CellIndex>& shelves, CellIndex end)
{
    RoutePlan plan;
    const std::vector<std::vector<CellIndex>> sides = sidesOf(shelves);
    std::vector<char> visited(shelves.size(), 0);
    CellIndex position = start;

    for (std::size_t step = 0; step < shelves.size(); ++step) {
        // Look at every (unvisited shelf, side) pair and take the closest one.
        int bestShelf = -1;
        CellIndex bestSide = kNoCell;
        int bestDistance = kUnreachable;
        for (std::size_t i = 0; i < shelves.size(); ++i) {
            if (visited[i]) continue;
            for (CellIndex a : sides[i]) {
                ++plan.nodes;
                const int d = dist_.distance(position, a);
                if (d < bestDistance) {
                    bestDistance = d;
                    bestShelf = static_cast<int>(i);
                    bestSide = a;
                }
            }
        }
        if (bestShelf < 0) break;   // remaining shelves unreachable (cannot happen on a validated map)
        visited[static_cast<std::size_t>(bestShelf)] = 1;
        plan.order.push_back(bestShelf);
        plan.access.push_back(bestSide);
        plan.length += bestDistance;
        position = bestSide;
    }
    plan.length += dist_.distance(position, end);
    return plan;
}

RoutePlan RoutePlanner::backtracking(CellIndex start, const std::vector<CellIndex>& shelves, CellIndex end, bool prune)
{
    SearchState s;
    s.sides = sidesOf(shelves);
    s.end = end;
    s.prune = prune;
    s.visited.assign(shelves.size(), 0);
    if (prune) {
        s.best = greedy(start, shelves, end);   // a good first "best so far" lets us prune early
        s.best.nodes = 0;
    } else {
        s.best.length = kUnreachable;           // no bound: explore the whole tree
    }
    search(s, start, 0);
    return s.best;
}

void RoutePlanner::search(SearchState& s, CellIndex position, int lengthSoFar)
{
    ++s.best.nodes;   // count every node of the search tree
    const std::size_t k = s.sides.size();

    if (s.order.size() == k) {                   // every shelf chosen: a complete route
        const int total = lengthSoFar + dist_.distance(position, s.end);
        if (total < s.best.length) {
            const long nodes = s.best.nodes;
            s.best.order = s.order;
            s.best.access = s.access;
            s.best.length = total;
            s.best.nodes = nodes;
        }
        return;
    }

    for (std::size_t i = 0; i < k; ++i) {
        if (s.visited[i]) continue;
        for (CellIndex a : s.sides[i]) {
            const int d = dist_.distance(position, a);
            if (d >= kUnreachable) continue;
            const int newLength = lengthSoFar + d;
            // Bound: from `a` we still have to reach the end, so the finished route is at
            // least newLength + distance(a, end). If that cannot beat the best, skip.
            if (s.prune && newLength + dist_.distance(a, s.end) >= s.best.length) continue;

            s.visited[i] = 1;                    // make the choice
            s.order.push_back(static_cast<int>(i));
            s.access.push_back(a);

            search(s, a, newLength);             // recurse

            s.visited[i] = 0;                    // undo the choice (backtrack)
            s.order.pop_back();
            s.access.pop_back();
        }
    }
}

} // namespace wh
