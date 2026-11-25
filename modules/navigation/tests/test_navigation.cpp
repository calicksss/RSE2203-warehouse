/**
 * @file test_navigation.cpp
 * @brief Tests for the navigation module: map loading, BFS, A*, DistanceTable, RoutePlanner, obstacles.
 *
 * Validation ideas used here:
 *  - a tiny map whose BFS distances you can check BY HAND (see the module README);
 *  - A* must give the same path LENGTH as BFS (BFS is simple, so we trust it);
 *  - backtracking must equal a brute-force search over every order (the definition of "optimal").
 */
#include <algorithm>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#include "TestFramework.hpp"
#include "core/Errors.hpp"
#include "navigation/DistanceTable.hpp"
#include "navigation/GridMap.hpp"
#include "navigation/ObstacleManager.hpp"
#include "navigation/Pathfinding.hpp"
#include "navigation/RoutePlanner.hpp"

using namespace wh;

namespace {

// The tiny map from the README hand-trace exercise.
//      col: 0123456
//   row 0   WWWWWWW
//   row 1   WT...CW
//   row 2   W.WSW.W
//   row 3   W.....W
//   row 4   WR....W
//   row 5   WWWWWWW
GridMap tinyMap()
{
    std::istringstream in("WWWWWWW\nWT...CW\nW.WSW.W\nW.....W\nWR....W\nWWWWWWW\n");
    return GridMap::parse(in, "tiny");
}

GridMap realMap() { return GridMap::loadFromFile(std::string(WH_DATA_DIR) + "/Map.txt"); }

GridMap parseText(const std::string& text)
{
    std::istringstream in(text);
    return GridMap::parse(in, "test");
}

std::vector<CellIndex> walkableCells(const GridMap& map)
{
    std::vector<CellIndex> cells;
    for (CellIndex c = 0; c < map.cellCount(); ++c)
        if (map.isWalkable(c)) cells.push_back(c);
    return cells;
}

/// A path is valid if every step goes to an adjacent walkable cell and it ends at the goal.
bool isValidPath(const GridMap& map, CellIndex start, CellIndex goal, const std::vector<CellIndex>& path)
{
    CellIndex previous = start;
    for (CellIndex c : path) {
        if (!map.isWalkable(c) || manhattan(map, previous, c) != 1) return false;
        previous = c;
    }
    return previous == goal;
}

/// Shortest route by trying EVERY order and EVERY side (brute force). Only for small k.
int bruteForceRoute(const GridMap& map, DistanceTable& dist, CellIndex start, std::vector<CellIndex> shelves, CellIndex end)
{
    std::sort(shelves.begin(), shelves.end());
    int best = kUnreachable;
    do {
        // For a fixed order, try every combination of sides (2^k or more).
        std::vector<std::vector<CellIndex>> sides;
        for (CellIndex s : shelves) sides.push_back(map.accessCells(s));
        std::vector<std::size_t> pick(shelves.size(), 0);
        while (true) {
            int length = 0;
            CellIndex pos = start;
            for (std::size_t i = 0; i < shelves.size(); ++i) {
                length += dist.distance(pos, sides[i][pick[i]]);
                pos = sides[i][pick[i]];
            }
            length += dist.distance(pos, end);
            best = std::min(best, length);
            std::size_t j = 0;   // next side combination (like counting in mixed base)
            while (j < pick.size() && ++pick[j] == sides[j].size()) pick[j++] = 0;
            if (j == pick.size()) break;
        }
    } while (std::next_permutation(shelves.begin(), shelves.end()));
    return best;
}

/// Recomputes a plan's length from its legs and checks it visits every shelf once from a real side.
bool planIsConsistent(const GridMap& map, DistanceTable& dist, CellIndex start, const std::vector<CellIndex>& shelves,
                      CellIndex end, const RoutePlan& plan)
{
    if (plan.order.size() != shelves.size() || plan.access.size() != shelves.size()) return false;
    std::vector<int> sortedOrder = plan.order;
    std::sort(sortedOrder.begin(), sortedOrder.end());
    for (std::size_t i = 0; i < sortedOrder.size(); ++i)
        if (sortedOrder[i] != static_cast<int>(i)) return false;   // each shelf exactly once
    int length = 0;
    CellIndex pos = start;
    for (std::size_t i = 0; i < plan.order.size(); ++i) {
        const CellIndex shelf = shelves[static_cast<std::size_t>(plan.order[i])];
        if (manhattan(map, shelf, plan.access[i]) != 1) return false;   // stops next to its shelf
        length += dist.distance(pos, plan.access[i]);
        pos = plan.access[i];
    }
    length += dist.distance(pos, end);
    return length == plan.length;
}

std::vector<CellIndex> randomShelves(const GridMap& map, std::mt19937& rng, int k)
{
    std::vector<CellIndex> all = map.shelves();
    std::shuffle(all.begin(), all.end(), rng);
    return std::vector<CellIndex>(all.begin(), all.begin() + k);
}

} // namespace

// ---------------------------------------------------------------------------
// GridMap
// ---------------------------------------------------------------------------

TEST_CASE("GridMap: the real map has the expected size and features")
{
    const GridMap map = realMap();
    CHECK(map.rows() == 21);
    CHECK(map.cols() == 33);
    CHECK(map.shelves().size() == 91);
    CHECK(map.sorters().size() == 4);
    CHECK(map.docks().size() == 4);
    CHECK(map.homes().size() == 8);
    for (CellIndex r : map.sorters()) CHECK(map.inductionCellFor(r) != kNoCell);
}

TEST_CASE("GridMap: index and pos are inverses")
{
    const GridMap map = realMap();
    for (CellIndex c = 0; c < map.cellCount(); ++c) CHECK(map.index(map.pos(c)) == c);
    CHECK(map.index({2, 5}) == 2 * 33 + 5);
}

TEST_CASE("GridMap: invalid maps are rejected")
{
    CHECK_THROWS(parseText("WWW\nWT\nWWW\n"));                              // not a rectangle
    CHECK_THROWS(parseText("WWWW\nWTXW\nWWWW\n"));                          // unknown symbol
    CHECK_THROWS(parseText("WWWWW\nW.SRW\nW.C.W\nWWWWW\n"));                // no home 'T'
    CHECK_THROWS(parseText("WWWWWWW\nWTC.R.W\nWWWWWWW\nWWWSWWW\nWWWWWWW\n"));  // shelf walled in
    CHECK_THROWS(parseText(""));                                           // empty
    const GridMap ok = parseText("WWWWWW\nWTSRCW\nW....W\nWWWWWW\n");
    CHECK(ok.shelves().size() == 1);
}

TEST_CASE("GridMap: neighbours skips walls, shelves and sorters")
{
    const GridMap map = tinyMap();
    CellIndex nb[4];
    CHECK(map.neighbours(map.index({1, 1}), nb) == 2);   // T: right (1,2) and down (2,1)
    CHECK(map.neighbours(map.index({1, 3}), nb) == 2);   // above the shelf: left and right only
    CHECK(map.accessCells(map.index({2, 3})).size() == 2);   // shelf reachable from above and below
}

// ---------------------------------------------------------------------------
// BFS and A*
// ---------------------------------------------------------------------------

TEST_CASE("BFS: distances on the tiny map match the hand-worked values in the README")
{
    const GridMap map = tinyMap();
    const std::vector<int> d = bfsDistances(map, map.index({1, 1}));
    auto at = [&](int r, int c) { return d[static_cast<std::size_t>(map.index({r, c}))]; };
    CHECK(at(1, 1) == 0);
    CHECK(at(1, 2) == 1);
    CHECK(at(2, 1) == 1);
    CHECK(at(1, 3) == 2);
    CHECK(at(3, 1) == 2);
    CHECK(at(1, 5) == 4);   // the dock
    CHECK(at(3, 3) == 4);   // below the shelf
    CHECK(at(4, 5) == 7);   // farthest cell
    CHECK(at(2, 3) == kUnreachable);   // the shelf itself is not walkable
    CHECK(at(0, 0) == kUnreachable);   // wall
}

TEST_CASE("A*: path length equals the BFS distance for every pair of cells on the tiny map")
{
    const GridMap map = tinyMap();
    for (CellIndex a : walkableCells(map)) {
        const std::vector<int> d = bfsDistances(map, a);
        for (CellIndex b : walkableCells(map)) {
            const PathResult p = aStarPath(map, a, b);
            REQUIRE(p.found);
            CHECK(static_cast<int>(p.path.size()) == d[static_cast<std::size_t>(b)]);
            CHECK(isValidPath(map, a, b, p.path));
        }
    }
}

TEST_CASE("A*: same length as BFS on 500 random pairs of the real map, and fewer cells expanded overall")
{
    const GridMap map = realMap();
    const std::vector<CellIndex> cells = walkableCells(map);
    std::mt19937 rng(1);
    std::uniform_int_distribution<std::size_t> pick(0, cells.size() - 1);
    long astarExpanded = 0, bfsExpanded = 0;
    for (int i = 0; i < 500; ++i) {
        const CellIndex a = cells[pick(rng)], b = cells[pick(rng)];
        const PathResult viaAStar = aStarPath(map, a, b);
        const PathResult viaBfs = bfsPath(map, a, b);
        REQUIRE(viaAStar.found && viaBfs.found);
        CHECK(viaAStar.path.size() == viaBfs.path.size());
        CHECK(isValidPath(map, a, b, viaAStar.path));
        CHECK(isValidPath(map, a, b, viaBfs.path));
        astarExpanded += viaAStar.expanded;
        bfsExpanded += viaBfs.expanded;
    }
    CHECK(astarExpanded < bfsExpanded);
}

TEST_CASE("A*: detours around blocked cells, and reports 'not found' when walled in")
{
    const GridMap map = tinyMap();
    const CellIndex home = map.index({1, 1}), dock = map.index({1, 5});
    CHECK(aStarPath(map, home, dock).path.size() == 4);   // straight along row 1

    std::vector<char> blocked(static_cast<std::size_t>(map.cellCount()), 0);
    blocked[static_cast<std::size_t>(map.index({1, 3}))] = 1;   // block the top corridor
    const PathResult detour = aStarPath(map, home, dock, blocked);
    REQUIRE(detour.found);
    CHECK(detour.path.size() == 8);                        // down, along row 3, up column 5
    for (CellIndex c : detour.path) CHECK(blocked[static_cast<std::size_t>(c)] == 0);
    CHECK(static_cast<int>(detour.path.size()) == bfsDistances(map, home, blocked)[static_cast<std::size_t>(dock)]);

    blocked[static_cast<std::size_t>(map.index({1, 2}))] = 1;
    blocked[static_cast<std::size_t>(map.index({2, 1}))] = 1;   // home is now boxed in
    CHECK(!aStarPath(map, home, dock, blocked).found);
}

TEST_CASE("A*: a blocked GOAL is still reachable (a robot may be standing on it for a moment)")
{
    const GridMap map = tinyMap();
    std::vector<char> blocked(static_cast<std::size_t>(map.cellCount()), 0);
    const CellIndex goal = map.index({3, 3});
    blocked[static_cast<std::size_t>(goal)] = 1;
    CHECK(aStarPath(map, map.index({1, 1}), goal, blocked).found);
}

// ---------------------------------------------------------------------------
// DistanceTable
// ---------------------------------------------------------------------------

TEST_CASE("DistanceTable: equals BFS, is symmetric, and reuses cached BFS results")
{
    const GridMap map = realMap();
    DistanceTable table(map);
    const CellIndex a = map.homes()[0];
    const std::vector<int> d = bfsDistances(map, a);
    for (CellIndex b : walkableCells(map)) CHECK(table.distance(a, b) == d[static_cast<std::size_t>(b)]);
    CHECK(table.bfsRuns() == 1);                 // one BFS answered all of those
    const CellIndex b = map.homes()[5];
    CHECK(table.distance(b, a) == table.distance(a, b));
    CHECK(table.bfsRuns() == 1);                 // reverse question also answered from the cache
}

// ---------------------------------------------------------------------------
// RoutePlanner
// ---------------------------------------------------------------------------

TEST_CASE("RoutePlanner: backtracking equals brute force on 60 random trips (k = 1..5)")
{
    const GridMap map = realMap();
    DistanceTable dist(map);
    RoutePlanner planner(map, dist);
    std::mt19937 rng(42);
    for (int trial = 0; trial < 60; ++trial) {
        const int k = 1 + trial % 5;
        const std::vector<CellIndex> shelves = randomShelves(map, rng, k);
        const CellIndex start = map.homes()[static_cast<std::size_t>(trial) % map.homes().size()];
        const CellIndex end = map.inductionCellFor(map.sorters()[static_cast<std::size_t>(trial) % map.sorters().size()]);
        const RoutePlan exact = planner.backtracking(start, shelves, end);
        CHECK(exact.length == bruteForceRoute(map, dist, start, shelves, end));
        CHECK(planIsConsistent(map, dist, start, shelves, end, exact));
    }
}

TEST_CASE("RoutePlanner: greedy and as-given are never shorter than backtracking, and plans are consistent")
{
    const GridMap map = realMap();
    DistanceTable dist(map);
    RoutePlanner planner(map, dist);
    std::mt19937 rng(7);
    for (int trial = 0; trial < 100; ++trial) {
        const std::vector<CellIndex> shelves = randomShelves(map, rng, 2 + trial % 6);
        const CellIndex start = map.homes()[0];
        const CellIndex end = map.inductionCellFor(map.sorters()[1]);
        const RoutePlan exact = planner.backtracking(start, shelves, end);
        const RoutePlan greedy = planner.greedy(start, shelves, end);
        const RoutePlan given = planner.asGiven(start, shelves, end);
        CHECK(greedy.length >= exact.length);
        CHECK(given.length >= exact.length);
        CHECK(planIsConsistent(map, dist, start, shelves, end, greedy));
        CHECK(planIsConsistent(map, dist, start, shelves, end, given));
    }
}

TEST_CASE("RoutePlanner: greedy is sometimes NOT optimal (lecture 5b: greedy does not always give the best)")
{
    const GridMap map = realMap();
    DistanceTable dist(map);
    RoutePlanner planner(map, dist);
    std::mt19937 rng(3);
    int greedyWorse = 0;
    for (int trial = 0; trial < 200; ++trial) {
        const std::vector<CellIndex> shelves = randomShelves(map, rng, 5);
        const CellIndex start = map.homes()[static_cast<std::size_t>(trial) % 8];
        const CellIndex end = map.inductionCellFor(map.sorters()[0]);
        if (planner.greedy(start, shelves, end).length > planner.backtracking(start, shelves, end).length) ++greedyWorse;
    }
    CHECK(greedyWorse > 0);   // at least one counter-example exists
}

TEST_CASE("RoutePlanner: pruning gives the same answer with far fewer search nodes")
{
    const GridMap map = realMap();
    DistanceTable dist(map);
    RoutePlanner planner(map, dist);
    std::mt19937 rng(9);
    long nodesPruned = 0, nodesFull = 0;
    for (int trial = 0; trial < 10; ++trial) {
        const std::vector<CellIndex> shelves = randomShelves(map, rng, 5);
        const CellIndex start = map.homes()[2];
        const CellIndex end = map.inductionCellFor(map.sorters()[2]);
        const RoutePlan pruned = planner.backtracking(start, shelves, end, true);
        const RoutePlan full = planner.backtracking(start, shelves, end, false);
        CHECK(pruned.length == full.length);
        nodesPruned += pruned.nodes;
        nodesFull += full.nodes;
    }
    CHECK(nodesPruned * 5 < nodesFull);   // at least 5x less work
}

TEST_CASE("RoutePlanner: plan() uses greedy above the exact limit and handles zero shelves")
{
    const GridMap map = realMap();
    DistanceTable dist(map);
    RoutePlanner planner(map, dist);
    std::mt19937 rng(11);
    const std::vector<CellIndex> shelves = randomShelves(map, rng, 9);
    const CellIndex start = map.homes()[0];
    const CellIndex end = map.inductionCellFor(map.sorters()[0]);
    const RoutePlan viaPlan = planner.plan(RoutingMethod::Backtracking, 7, start, shelves, end);
    const RoutePlan viaGreedy = planner.greedy(start, shelves, end);
    CHECK(viaPlan.length == viaGreedy.length);
    CHECK(viaPlan.order == viaGreedy.order);

    const RoutePlan empty = planner.backtracking(start, {}, end);
    CHECK(empty.order.empty());
    CHECK(empty.length == dist.distance(start, end));
}

// ---------------------------------------------------------------------------
// ObstacleManager
// ---------------------------------------------------------------------------

TEST_CASE("Obstacles: never on protected cells or robots, never more than the maximum, and they expire")
{
    const GridMap map = realMap();
    std::vector<CellIndex> protectedCells(map.homes().begin(), map.homes().end());
    for (CellIndex s : map.shelves())
        for (CellIndex a : map.accessCells(s)) protectedCells.push_back(a);
    ObstacleManager obstacles(map, protectedCells);
    SimConfig config;
    config.obstacleSpawnChance = 1.0;   // try to spawn every tick
    config.maxObstacles = 5;
    std::mt19937 rng(3);
    EventLog log;
    const std::vector<CellIndex> robots{map.index({5, 5}), map.index({6, 6})};

    for (Tick t = 1; t <= 300; ++t) {
        obstacles.update(t, config, rng, robots, log);
        CHECK(static_cast<int>(obstacles.active().size()) <= config.maxObstacles);
        for (const Obstacle& o : obstacles.active()) {
            CHECK(map.tile(o.cell) == Tile::Floor);
            CHECK(std::find(protectedCells.begin(), protectedCells.end(), o.cell) == protectedCells.end());
            CHECK(std::find(robots.begin(), robots.end(), o.cell) == robots.end());
            CHECK(o.expires > t);
        }
    }
    config.randomObstacles = false;
    obstacles.update(10000, config, rng, robots, log);   // far in the future: all expired
    CHECK(obstacles.active().empty());
    for (char blocked : obstacles.mask()) CHECK(blocked == 0);
}

// ---------------------------------------------------------------------------
// WP-C additions: gaps found by my own mutants (see docs/GENAI_LOG.md)
// ---------------------------------------------------------------------------

TEST_CASE("BFS path: a blocked GOAL is still reachable (same rule as A*)")
{
    const GridMap map = tinyMap();
    std::vector<char> blocked(static_cast<std::size_t>(map.cellCount()), 0);
    const CellIndex goal = map.index({3, 3});
    blocked[static_cast<std::size_t>(goal)] = 1;
    const PathResult viaBfs = bfsPath(map, map.index({1, 1}), goal, blocked);
    const PathResult viaAStar = aStarPath(map, map.index({1, 1}), goal, blocked);
    CHECK(viaBfs.found);
    CHECK(viaBfs.path.size() == viaAStar.path.size());
}

TEST_CASE("DistanceTable: shelves, sorters and walls are not vertices (answer is kUnreachable both ways)")
{
    const GridMap map = tinyMap();
    DistanceTable table(map);
    const CellIndex shelf = map.index({2, 3}), wall = map.index({0, 0}), sorter = map.index({4, 1}), home = map.index({1, 1});
    CHECK(table.distance(shelf, home) == kUnreachable);
    CHECK(table.distance(home, shelf) == kUnreachable);
    CHECK(table.distance(wall, home) == kUnreachable);
    CHECK(table.distance(sorter, home) == kUnreachable);
    CHECK(table.distance(home, home) == 0);
    CHECK(table.bfsRuns() <= 1);   // the rejected questions must not cost a BFS
}

TEST_CASE("RoutePlanner: plan() switches from backtracking to greedy exactly at exactLimit")
{
    const GridMap map = realMap();
    DistanceTable dist(map);
    RoutePlanner planner(map, dist);
    std::mt19937 rng(21);
    const CellIndex end = map.inductionCellFor(map.sorters()[0]);
    bool tested = false;
    for (int trial = 0; trial < 200 && !tested; ++trial) {
        const std::vector<CellIndex> shelves = randomShelves(map, rng, 4);
        const CellIndex start = map.homes()[static_cast<std::size_t>(trial) % 8];
        const RoutePlan greedy = planner.greedy(start, shelves, end);
        const RoutePlan exact = planner.backtracking(start, shelves, end);
        if (greedy.length == exact.length) continue;               // need a trip where the two methods differ
        CHECK(planner.plan(RoutingMethod::Backtracking, 4, start, shelves, end).length == exact.length);    // k == limit: exact
        CHECK(planner.plan(RoutingMethod::Backtracking, 3, start, shelves, end).length == greedy.length);   // k  > limit: greedy
        tested = true;
    }
    REQUIRE(tested);
}

TEST_CASE("GridMap: an unreachable dock is rejected, a reachable one is accepted")
{
    const std::string base = "WWWWWWWW\nWT....SW\nW.....RW\nWWW%WWWW\nWWWCWWWW\nWWWWWWWW\n";
    std::string connected = base, cut = base;
    connected[connected.find('%')] = '.';
    cut[cut.find('%')] = 'W';
    CHECK_THROWS(parseText(cut));
    CHECK(parseText(connected).docks().size() == 1);
}

TEST_CASE("GridMap: sorter induction cells on the real map are the open floor cells the other modules rely on")
{
    const GridMap map = realMap();
    // sorters at (1,16) (1,31) (19,1) (19,16): never a dock or a robot home, and always a plain floor cell next to the sorter
    for (CellIndex r : map.sorters()) {
        const CellIndex in = map.inductionCellFor(r);
        REQUIRE(in != kNoCell);
        CHECK(map.tile(in) == Tile::Floor);
        CHECK(!map.isHome(in));
        CHECK(manhattan(map, r, in) == 1);
    }
    CHECK(map.inductionCellFor(map.index({1, 16})) == map.index({2, 16}));
    CHECK(map.inductionCellFor(map.index({1, 31})) == map.index({1, 30}));
    CHECK(map.inductionCellFor(map.index({19, 1})) == map.index({19, 2}));
    CHECK(map.inductionCellFor(map.index({19, 16})) == map.index({18, 16}));
}
// ---------------------------------------------------------------------------
// WP-C fixes F2 and F5: BFS and A* must give the same answers
// ---------------------------------------------------------------------------

TEST_CASE("BFS distances: a blocked cell is reachable as an END point, same as bfsPath and A*")
{
    const GridMap map = tinyMap();
    const CellIndex home = map.index({1, 1});
    for (CellIndex target = 0; target < map.cellCount(); ++target) {
        if (!map.isWalkable(target) || target == home) continue;
        std::vector<char> blocked(static_cast<std::size_t>(map.cellCount()), 0);
        blocked[static_cast<std::size_t>(target)] = 1;
        const PathResult viaBfs = bfsPath(map, home, target, blocked);
        const PathResult viaAStar = aStarPath(map, home, target, blocked);
        REQUIRE(viaBfs.found);
        REQUIRE(viaAStar.found);
        CHECK(bfsDistances(map, home, blocked)[static_cast<std::size_t>(target)] == static_cast<int>(viaBfs.path.size()));
        CHECK(viaBfs.path.size() == viaAStar.path.size());
    }
}

TEST_CASE("BFS distances: a blocked cell is still never walked THROUGH")
{
    const GridMap map = tinyMap();
    const CellIndex home = map.index({1, 1}), dock = map.index({1, 5});
    std::vector<char> blocked(static_cast<std::size_t>(map.cellCount()), 0);
    blocked[static_cast<std::size_t>(map.index({1, 3}))] = 1;   // top corridor
    const std::vector<int> dist = bfsDistances(map, home, blocked);
    CHECK(dist[static_cast<std::size_t>(map.index({1, 3}))] == 2);   // the blocked cell itself can be reached
    CHECK(dist[static_cast<std::size_t>(dock)] == 8);                // but the way to the dock goes round it
}

TEST_CASE("BFS path and A*: a start or goal that is not a walkable cell gives 'not found', like DistanceTable")
{
    const GridMap map = tinyMap();
    DistanceTable table(map);
    const CellIndex home = map.index({1, 1});
    const CellIndex shelf = map.index({2, 3}), wall = map.index({0, 0}), sorter = map.index({4, 1});
    for (CellIndex bad : {shelf, wall, sorter}) {
        CHECK(!aStarPath(map, bad, home).found);
        CHECK(!aStarPath(map, home, bad).found);
        CHECK(!aStarPath(map, bad, bad).found);
        CHECK(!bfsPath(map, bad, home).found);
        CHECK(!bfsPath(map, home, bad).found);
        CHECK(!bfsPath(map, bad, bad).found);
        CHECK(table.distance(bad, home) == kUnreachable);
    }
    CHECK(aStarPath(map, home, home).found);   // a walkable start that is also the goal: found, empty path
    CHECK(aStarPath(map, home, home).path.empty());
}