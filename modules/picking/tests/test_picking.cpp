/**
 * @file test_picking.cpp
 * @brief Tests for the picking module: battery, picking-robot state machine and traffic rules.
 */
#include <algorithm>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "TestFramework.hpp"
#include "navigation/ObstacleManager.hpp"
#include "navigation/Pathfinding.hpp"
#include "picking/PickingRobot.hpp"
#include "picking/TrafficController.hpp"

using namespace wh;

namespace {

/// Records what robots report, instead of a real JobManager (a "test double").
struct RecordingListener : FulfilmentListener {
    int picked = 0;
    int delivered = 0;
    int deliveredToSorter = -1;
    void onUnitsPicked(const std::vector<PickUnit>& units, Tick) override { picked += static_cast<int>(units.size()); }
    void onUnitsDelivered(int sorterIndex, const std::vector<PickUnit>& units, Tick) override
    {
        delivered += static_cast<int>(units.size());
        deliveredToSorter = sorterIndex;
    }
    void onUnitSorted(const PickUnit&, Tick) override {}
    void onOrderPacked(OrderId, const std::vector<PickUnit>&, int, Tick) override {}
};

GridMap parseMap(const std::string& text)
{
    std::istringstream in(text);
    return GridMap::parse(in, "test");
}

// Tiny map (same as the navigation tests): home (1,1), shelf (2,3), dock (1,5), sorter (4,1).
const char* kTiny = "WWWWWWW\nWT...CW\nW.WSW.W\nW.....W\nWR....W\nWWWWWWW\n";

// A corridor with one side pocket at (2,4): two robots meeting head-on must use it.
const char* kCorridor =
    "WWWWWWWWW\n"
    "WT.....TW\n"
    "WWWW.WWWW\n"
    "WWR..CWWW\n"
    "WWWSWWWWW\n"
    "WWWWWWWWW\n";

/// Runs traffic + robot ticks until every robot stops moving (or maxTicks). Checks safety every tick.
struct World {
    GridMap map;
    SimConfig config;
    ObstacleManager obstacles;
    TrafficController traffic;
    EventLog log;
    Metrics metrics;
    RecordingListener listener;
    std::vector<std::unique_ptr<PickingRobot>> owned;
    std::vector<PickingRobot*> robots;
    bool safe = true;

    World(GridMap m, std::vector<CellIndex> protectedCells)
        : map(std::move(m)), obstacles(map, protectedCells), traffic(map, config)
    {
        config.randomObstacles = false;
    }

    PickingRobot& add(CellIndex home)
    {
        owned.push_back(std::make_unique<PickingRobot>(static_cast<int>(owned.size()) + 1, static_cast<int>(owned.size()) + 1, home, 100.0));
        robots.push_back(owned.back().get());
        return *owned.back();
    }

    int run(int maxTicks)
    {
        for (int t = 1; t <= maxTicks; ++t) {
            std::vector<CellIndex> before;
            for (PickingRobot* r : robots) before.push_back(r->cell());
            traffic.step(robots, obstacles, t, log, metrics);
            TickContext ctx{t, config, log, listener};
            for (PickingRobot* r : robots) r->tick(ctx);

            std::set<CellIndex> seen;
            for (std::size_t i = 0; i < robots.size(); ++i) {
                const CellIndex c = robots[i]->cell();
                if (!seen.insert(c).second) safe = false;                         // two robots in one cell
                if (!map.isWalkable(c) || obstacles.isBlocked(c)) safe = false;  // on a wall or obstacle
                if (c != before[i] && manhattan(map, c, before[i]) != 1) safe = false;   // jumped
                for (std::size_t j = 0; j < robots.size(); ++j)                  // swapped places
                    if (i != j && c == before[j] && robots[j]->cell() == before[i] && c != before[i]) safe = false;
            }
            bool anyMoving = false;
            for (PickingRobot* r : robots) anyMoving = anyMoving || r->wantsToMove() || r->isBusy();
            if (!anyMoving) return t;
        }
        return -1;
    }
};

PickStop stopAt(const GridMap& map, GridPos shelf, GridPos access, int units)
{
    PickStop s;
    s.shelf = map.index(shelf);
    s.access = map.index(access);
    for (int i = 0; i < units; ++i) s.units.push_back({1, 0, s.shelf, 1.0, 0});
    return s;
}

} // namespace

// ---------------------------------------------------------------------------
// Battery
// ---------------------------------------------------------------------------

TEST_CASE("Battery: stays between 0 and 100")
{
    Battery b(150.0);
    CHECK(b.percent() == 100.0);
    b.drain(130.0);
    CHECK(b.percent() == 0.0);
    CHECK(b.isEmpty());
    b.charge(30.0);
    CHECK(b.percent() == 30.0);
}

// ---------------------------------------------------------------------------
// PickingRobot state machine
// ---------------------------------------------------------------------------

TEST_CASE("PickingRobot: goes shelf -> shelf -> sorter -> home and reports picks and the delivery")
{
    World w(parseMap(kTiny), {});
    PickingRobot& r = w.add(w.map.index({1, 1}));
    CHECK(r.state() == PickingRobot::State::Idle);
    CHECK(r.isAvailable());

    std::vector<PickStop> stops{stopAt(w.map, {2, 3}, {1, 3}, 2), stopAt(w.map, {2, 3}, {3, 3}, 1)};
    r.assignTrip(stops, w.map.index({3, 1}), 0, 1);
    CHECK(r.state() == PickingRobot::State::ToShelf);
    CHECK(r.goal() == w.map.index({1, 3}));
    CHECK(!r.isAvailable());
    CHECK_THROWS(r.assignTrip(stops, w.map.index({3, 1}), 0, 2));   // already busy

    const int ticks = w.run(200);
    CHECK(ticks > 0);
    CHECK(w.safe);
    CHECK(w.listener.picked == 3);
    CHECK(w.listener.delivered == 3);
    CHECK(w.listener.deliveredToSorter == 0);
    CHECK(r.state() == PickingRobot::State::Idle);
    CHECK(r.cell() == r.home());
    CHECK(r.cargo().empty());
}

TEST_CASE("PickingRobot: a trip paused for charging resumes afterwards")
{
    World w(parseMap(kTiny), {});
    DockingStation dock(0, w.map.index({1, 5}), 10.0);
    PickingRobot& r = w.add(w.map.index({1, 1}));
    r.assignTrip({stopAt(w.map, {2, 3}, {3, 3}, 1)}, w.map.index({3, 1}), 0, 1);
    REQUIRE(dock.reserve(r.id()));
    r.sendToDock(dock);
    CHECK(r.isCharging());
    CHECK(r.goal() == dock.cell());

    CHECK(w.run(300) > 0);
    CHECK(w.safe);
    CHECK(w.listener.delivered == 1);   // the paused trip was finished after charging
    CHECK(dock.isFree());               // and the dock was released
    CHECK(r.battery() >= w.config.chargeTarget - 5.0);
}

TEST_CASE("PickingRobot: a parked robot that is moved drives back home")
{
    World w(parseMap(kTiny), {});
    PickingRobot& r = w.add(w.map.index({1, 1}));
    r.moveTo(w.map.index({1, 2}), 0.0);
    CHECK(r.state() == PickingRobot::State::Returning);
    CHECK(w.run(50) > 0);
    CHECK(r.state() == PickingRobot::State::Idle);
    CHECK(r.cell() == w.map.index({1, 1}));
}

// ---------------------------------------------------------------------------
// TrafficController
// ---------------------------------------------------------------------------

TEST_CASE("Traffic: a single robot drives a shortest path (ticks moved = BFS distance)")
{
    World w(parseMap(kTiny), {});
    PickingRobot& r = w.add(w.map.index({4, 5}));   // "home" in the far corner
    r.moveTo(w.map.index({1, 1}), 0.0);              // put it at the top-left: it now drives home
    const long movesBefore = r.moves();
    CHECK(w.run(50) > 0);
    CHECK(r.moves() - movesBefore == bfsDistances(w.map, w.map.index({1, 1}))[static_cast<std::size_t>(w.map.index({4, 5}))]);
    CHECK(w.metrics.waits == 0);
}

TEST_CASE("Traffic: two robots meeting head-on in a corridor pass using the side pocket, without collision")
{
    World w(parseMap(kCorridor), {});
    const CellIndex left = w.map.index({1, 1}), right = w.map.index({1, 7});
    PickingRobot& a = w.add(right);   // A lives on the right ...
    PickingRobot& b = w.add(left);    // ... B on the left
    a.moveTo(left, 0.0);              // swap them: each must drive through the other
    b.moveTo(right, 0.0);
    const int ticks = w.run(300);
    CHECK(ticks > 0);
    CHECK(w.safe);
    CHECK(a.cell() == right);
    CHECK(b.cell() == left);
}

TEST_CASE("Traffic: a parked robot in the way is nudged aside and goes home again")
{
    World w(parseMap(kTiny), {});
    const CellIndex dock = w.map.index({1, 5});
    PickingRobot& mover = w.add(w.map.index({1, 1}));
    PickingRobot& parked = w.add(w.map.index({1, 3}));   // parked in the top corridor
    DockingStation station(0, dock, 50.0);
    station.reserve(mover.id());
    mover.sendToDock(station);
    CHECK(w.run(300) > 0);
    CHECK(w.safe);
    CHECK(station.isFree());                       // mover reached the dock, charged and left
    CHECK(w.metrics.nudges >= 1);
    CHECK(parked.cell() == parked.home());         // and the parked robot is home again
}

TEST_CASE("Traffic: a new obstacle on the route causes a detour and is never entered")
{
    GridMap map = parseMap(kTiny);
    // Protect every floor cell except (1,3), so the only place an obstacle can appear is (1,3).
    std::vector<CellIndex> protectedCells;
    for (CellIndex c = 0; c < map.cellCount(); ++c)
        if (map.tile(c) == Tile::Floor && c != map.index({1, 3})) protectedCells.push_back(c);
    World w(map, protectedCells);
    PickingRobot& r = w.add(w.map.index({1, 5}));   // home at the dock cell's side
    r.moveTo(w.map.index({1, 1}), 0.0);              // drives right along row 1 ...
    std::mt19937 rng(1);
    w.config.obstacleMinLife = w.config.obstacleMaxLife = 1000;
    REQUIRE(w.obstacles.spawnRandom(0, w.config, rng, {r.cell()}, w.log));   // ... but (1,3) is now blocked
    REQUIRE(w.obstacles.isBlocked(w.map.index({1, 3})));

    const long movesBefore = r.moves();
    CHECK(w.run(100) > 0);
    CHECK(w.safe);                                   // never stood on the obstacle
    CHECK(r.cell() == w.map.index({1, 5}));
    CHECK(r.moves() - movesBefore == 8);             // the detour along row 3
}

TEST_CASE("Traffic: eight robots criss-crossing a small map never collide and all arrive")
{
    // Open 7x9 floor with homes on both sides.
    const char* text =
        "WWWWWWWWWWW\n"
        "WT.......TW\n"
        "WT.......TW\n"
        "WT.......TW\n"
        "WT.......TW\n"
        "WWSWWRWWCWW\n"
        "WWWWWWWWWWW\n";
    World w(parseMap(text), {});
    std::vector<CellIndex> left, right;
    for (int row = 1; row <= 4; ++row) {
        left.push_back(w.map.index({row, 1}));
        right.push_back(w.map.index({row, 9}));
    }
    // Robots living on the left start on the right and vice versa (reversed rows too).
    for (int k = 0; k < 4; ++k) {
        w.add(left[static_cast<std::size_t>(k)]).moveTo(right[static_cast<std::size_t>(3 - k)], 0.0);
        w.add(right[static_cast<std::size_t>(k)]).moveTo(left[static_cast<std::size_t>(3 - k)], 0.0);
    }
    CHECK(w.run(500) > 0);
    CHECK(w.safe);
    for (PickingRobot* r : w.robots) CHECK(r->cell() == r->home());
}
