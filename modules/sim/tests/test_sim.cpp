/**
 * @file test_sim.cpp
 * @brief Whole-system tests: many orders, obstacles, charging. Safety (no collisions) and liveness (all orders finish).
 */
#include <random>
#include <string>

#include "TestFramework.hpp"
#include "core/Errors.hpp"
#include "shop/Shop.hpp"
#include "sim/Simulation.hpp"

using namespace wh;

namespace {

const GridMap& realMap()
{
    static const GridMap map = GridMap::loadFromFile(std::string(WH_DATA_DIR) + "/Map.txt");
    return map;
}
const Catalogue& realCatalogue()
{
    static const Catalogue catalogue = Catalogue::loadFromFile(std::string(WH_DATA_DIR) + "/Products.txt");
    return catalogue;
}

struct RunResult {
    bool finished = false;
    Tick ticks = 0;
    Metrics metrics;
};

/// Submits `orders` random orders at tick 0 and runs until everything is done (or maxTicks).
RunResult runOrders(const SimConfig& config, int orders, Tick maxTicks = 20000)
{
    Shop shop(realCatalogue(), "");
    Simulation sim(realMap(), shop.catalogue(), config);
    std::mt19937 rng(config.seed);
    for (const Order& o : shop.generateOrders(orders, rng)) sim.submit(o);
    while (!sim.isQuiescent() && sim.now() < maxTicks) sim.step();
    return {sim.isQuiescent(), sim.now(), sim.metrics()};
}

} // namespace

TEST_CASE("Simulation: 80 orders with heavy obstacles all complete, with zero collisions")
{
    SimConfig config;
    config.obstacleSpawnChance = 0.2;
    config.maxObstacles = 8;
    const RunResult r = runOrders(config, 80);
    CHECK(r.finished);
    CHECK(r.metrics.ordersCompleted == 80);
    CHECK(r.metrics.collisions == 0);
    CHECK(r.metrics.unitsPicked == r.metrics.unitsSorted);
    CHECK(r.metrics.obstaclesSpawned > 0);
    CHECK(r.metrics.detours > 0);   // robots really did drive around obstacles
}

TEST_CASE("Simulation: every picker works (robots run in parallel, not one at a time)")
{
    SimConfig config;
    Shop shop(realCatalogue(), "");
    Simulation sim(realMap(), shop.catalogue(), config);
    std::mt19937 rng(1);
    for (const Order& o : shop.generateOrders(40, rng)) sim.submit(o);
    int maxBusyAtOnce = 0;
    while (!sim.isQuiescent() && sim.now() < 20000) {
        sim.step();
        int busy = 0;
        for (const PickingRobot* p : sim.pickers()) busy += p->hasWork() ? 1 : 0;
        maxBusyAtOnce = std::max(maxBusyAtOnce, busy);
    }
    CHECK(sim.allWorkDone());
    CHECK(maxBusyAtOnce == static_cast<int>(sim.pickers().size()));
    for (const PickingRobot* p : sim.pickers()) CHECK(p->moves() > 0);
}

TEST_CASE("Simulation: express orders finish faster on average than standard orders")
{
    SimConfig config;
    config.randomObstacles = false;
    Shop shop(realCatalogue(), "");
    Simulation sim(realMap(), shop.catalogue(), config);
    std::mt19937 rng(4);
    for (const Order& o : shop.generateOrders(60, rng, 0.3)) sim.submit(o);
    while (!sim.isQuiescent() && sim.now() < 20000) sim.step();
    REQUIRE(sim.allWorkDone());
    double expressSum = 0, standardSum = 0;
    int expressCount = 0, standardCount = 0;
    for (OrderId id : sim.jobs().orderIds()) {
        const OrderRecord& rec = *sim.jobs().record(id);
        const double lead = double(rec.completed - rec.submitted);
        if (rec.order.priority == Priority::Express) {
            expressSum += lead;
            ++expressCount;
        } else {
            standardSum += lead;
            ++standardCount;
        }
    }
    REQUIRE(expressCount > 0);
    REQUIRE(standardCount > 0);
    CHECK(expressSum / expressCount < standardSum / standardCount);
}

TEST_CASE("Simulation: with fast battery drain robots charge, resume, and still finish every order")
{
    SimConfig config;
    config.drainPerMove = 1.5;   // five times the normal drain
    const RunResult r = runOrders(config, 40, 40000);
    CHECK(r.finished);
    CHECK(r.metrics.ordersCompleted == 40);
    CHECK(r.metrics.chargeSessions > 0);
    CHECK(r.metrics.collisions == 0);
}

TEST_CASE("Simulation: the same seed gives exactly the same run (reproducible experiments)")
{
    SimConfig config;
    const RunResult a = runOrders(config, 30);
    const RunResult b = runOrders(config, 30);
    CHECK(a.ticks == b.ticks);
    CHECK(a.metrics.moves == b.metrics.moves);
    CHECK(a.metrics.waits == b.metrics.waits);
}

TEST_CASE("Simulation: stress - 48 runs (all methods, 1-8 pickers, obstacles) finish with no collisions")
{
    int runs = 0, ok = 0;
    for (unsigned seed = 1; seed <= 4; ++seed) {
        for (RoutingMethod routing : {RoutingMethod::AsGiven, RoutingMethod::Greedy, RoutingMethod::Backtracking}) {
            for (int pickers : {1, 3, 5, 8}) {
                SimConfig config;
                config.seed = seed;
                config.routing = routing;
                config.assignment = seed % 2 ? AssignmentMethod::Nearest : AssignmentMethod::FirstAvailable;
                config.maxPickers = pickers;
                config.obstacleSpawnChance = 0.1;
                config.maxObstacles = 6;
                const RunResult r = runOrders(config, 30, 40000);
                ++runs;
                if (r.finished && r.metrics.collisions == 0 && r.metrics.ordersCompleted == 30) ++ok;
            }
        }
    }
    CHECK(ok == runs);
}

TEST_CASE("Simulation: a product heavier than a picker's payload is rejected at start-up")
{
    SimConfig config;
    config.tripPayloadKg = 1.0;   // Bag of rice weighs 2 kg
    CHECK_THROWS(Simulation(realMap(), realCatalogue(), config));
}
